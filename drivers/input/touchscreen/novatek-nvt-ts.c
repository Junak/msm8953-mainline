// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for Novatek NT11205 i2c touchscreen controller as found
 * on the Acer Iconia One 7 B1-750 tablet.
 *
 * Copyright (c) 2023 Hans de Goede <hdegoede@redhat.com>
 */

#include <drm/drm_panel.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/module.h>
#include <linux/unaligned.h>

#define NVT_TS_TOUCH_START		0x00
#define NVT_TS_TOUCH_SIZE		6

#define NVT_TS_PARAMETERS_START		0x78
/* These are offsets from NVT_TS_PARAMETERS_START */
#define NVT_TS_PARAMS_WIDTH		0x04
#define NVT_TS_PARAMS_HEIGHT		0x06
#define NVT_TS_PARAMS_MAX_TOUCH		0x09
#define NVT_TS_PARAMS_MAX_BUTTONS	0x0a
#define NVT_TS_PARAMS_IRQ_TYPE		0x0b
#define NVT_TS_PARAMS_WAKE_TYPE		0x0c
#define NVT_TS_PARAMS_CHIP_ID		0x0e
#define NVT_TS_PARAMS_SIZE		0x0f

#define NVT_TS_MAX_TOUCHES		10
#define NVT_TS_MAX_SIZE			4096

#define NVT_TS_TOUCH_INVALID		0xff
#define NVT_TS_TOUCH_SLOT_SHIFT		3
#define NVT_TS_TOUCH_TYPE_MASK		GENMASK(2, 0)
#define NVT_TS_TOUCH_NEW		1
#define NVT_TS_TOUCH_UPDATE		2
#define NVT_TS_TOUCH_RELEASE		3

static const int nvt_ts_irq_type[4] = {
	IRQF_TRIGGER_RISING,
	IRQF_TRIGGER_FALLING,
	IRQF_TRIGGER_LOW,
	IRQF_TRIGGER_HIGH
};

struct nvt_ts_i2c_chip_data {
	u8 wake_type;
	u8 chip_id;
};

struct nvt_ts_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data regulators[2];
	struct touchscreen_properties prop;
	int max_touches;
	u8 buf[NVT_TS_TOUCH_SIZE * NVT_TS_MAX_TOUCHES];
	/*
	 * Sometimes Novatek touchscreen is paired together with Novatek panel,
	 * and they need to be powered together in sync.
	 */
	struct drm_panel_follower panel_follower;
	bool is_panel_follower;
};

static int nvt_ts_read_data(struct i2c_client *client, u8 reg, u8 *data, int count)
{
	struct i2c_msg msg[2] = {
		{
			.addr = client->addr,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = count,
			.buf = data,
		}
	};
	int ret;

	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev, "Error reading from 0x%02x: %d\n", reg, ret);
		return (ret < 0) ? ret : -EIO;
	}

	return 0;
}

static irqreturn_t nvt_ts_irq(int irq, void *dev_id)
{
	struct nvt_ts_data *data = dev_id;
	struct device *dev = &data->client->dev;
	int i, error, slot, x, y;
	bool active;
	u8 *touch;

	if (!data->input)
		return IRQ_HANDLED;

	error = nvt_ts_read_data(data->client, NVT_TS_TOUCH_START, data->buf,
				 data->max_touches * NVT_TS_TOUCH_SIZE);
	if (error)
		return IRQ_HANDLED;

	for (i = 0; i < data->max_touches; i++) {
		touch = &data->buf[i * NVT_TS_TOUCH_SIZE];

		if (touch[0] == NVT_TS_TOUCH_INVALID)
			continue;

		slot = touch[0] >> NVT_TS_TOUCH_SLOT_SHIFT;
		if (slot < 1 || slot > data->max_touches) {
			dev_dbg(dev, "slot %d out of range, ignoring\n", slot);
			continue;
		}

		switch (touch[0] & NVT_TS_TOUCH_TYPE_MASK) {
		case NVT_TS_TOUCH_NEW:
		case NVT_TS_TOUCH_UPDATE:
			active = true;
			break;
		case NVT_TS_TOUCH_RELEASE:
			active = false;
			break;
		default:
			dev_warn(dev, "slot %d unknown state %d\n", slot, touch[0] & 7);
			continue;
		}

		slot--;
		x = (touch[1] << 4) | (touch[3] >> 4);
		y = (touch[2] << 4) | (touch[3] & 0x0f);

		input_mt_slot(data->input, slot);
		input_mt_report_slot_state(data->input, MT_TOOL_FINGER, active);
		touchscreen_report_pos(data->input, &data->prop, x, y, true);
	}

	input_mt_sync_frame(data->input);
	input_sync(data->input);

	return IRQ_HANDLED;
}

static int nvt_ts_start(struct input_dev *dev)
{
	struct nvt_ts_data *data = input_get_drvdata(dev);
	int error;

	error = regulator_bulk_enable(ARRAY_SIZE(data->regulators), data->regulators);
	if (error) {
		dev_err(&data->client->dev, "failed to enable regulators\n");
		return error;
	}

	enable_irq(data->client->irq);
	gpiod_set_value_cansleep(data->reset_gpio, 0);
	msleep(100); // TODO: is it really needed? probably

	return 0;
}

static void nvt_ts_stop(struct input_dev *dev)
{
	struct nvt_ts_data *data = input_get_drvdata(dev);

	disable_irq(data->client->irq);
	gpiod_set_value_cansleep(data->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(data->regulators), data->regulators);
}

static int nvt_ts_suspend(struct device *dev)
{
	struct nvt_ts_data *data = i2c_get_clientdata(to_i2c_client(dev));

	if (data->is_panel_follower)
		return 0;

	mutex_lock(&data->input->mutex);
	if (input_device_enabled(data->input))
		nvt_ts_stop(data->input);
	mutex_unlock(&data->input->mutex);

	return 0;
}

static int nvt_ts_resume(struct device *dev)
{
	struct nvt_ts_data *data = i2c_get_clientdata(to_i2c_client(dev));

	if (data->is_panel_follower)
		return 0;

	mutex_lock(&data->input->mutex);
	if (input_device_enabled(data->input))
		nvt_ts_start(data->input);
	mutex_unlock(&data->input->mutex);

	return 0;
}

static int nvt_ts_initial_power_on_and_register_inputdev(struct nvt_ts_data *data)
{
	struct device *dev = &data->client->dev;
	int error, width, height, irq_type;
	const struct nvt_ts_i2c_chip_data *chip;
	struct input_dev *input;

	chip = device_get_match_data(dev);
	if (!chip)
		return -EINVAL;

	/*
	 * VCC is the analog voltage supply
	 * IOVCC is the digital voltage supply
	 */
	data->regulators[0].supply = "vcc";
	data->regulators[1].supply = "iovcc";
	error = devm_regulator_bulk_get(dev, ARRAY_SIZE(data->regulators), data->regulators);
	if (error) {
		dev_err(dev, "cannot get regulators: %d\n", error);
		return error;
	}

	error = regulator_bulk_enable(ARRAY_SIZE(data->regulators), data->regulators);
	if (error) {
		dev_err(dev, "failed to enable regulators: %d\n", error);
		return error;
	}

	data->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	error = PTR_ERR_OR_ZERO(data->reset_gpio);
	if (error) {
		regulator_bulk_disable(ARRAY_SIZE(data->regulators), data->regulators);
		dev_err(dev, "failed to request reset GPIO: %d\n", error);
		return error;
	}

	/* Wait for controller to come out of reset before params read */
	msleep(200);
	error = nvt_ts_read_data(data->client, NVT_TS_PARAMETERS_START,
				 data->buf, NVT_TS_PARAMS_SIZE);
	gpiod_set_value_cansleep(data->reset_gpio, 1); /* Put back in reset */
	regulator_bulk_disable(ARRAY_SIZE(data->regulators), data->regulators);
	if (error)
		return error;

	width  = get_unaligned_be16(&data->buf[NVT_TS_PARAMS_WIDTH]);
	height = get_unaligned_be16(&data->buf[NVT_TS_PARAMS_HEIGHT]);
	data->max_touches = data->buf[NVT_TS_PARAMS_MAX_TOUCH];
	irq_type = data->buf[NVT_TS_PARAMS_IRQ_TYPE];

	if (width > NVT_TS_MAX_SIZE || height >= NVT_TS_MAX_SIZE ||
	    data->max_touches > NVT_TS_MAX_TOUCHES ||
	    irq_type >= ARRAY_SIZE(nvt_ts_irq_type) ||
	    data->buf[NVT_TS_PARAMS_WAKE_TYPE] != chip->wake_type ||
	    data->buf[NVT_TS_PARAMS_CHIP_ID] != chip->chip_id) {
		dev_err(dev, "Unsupported touchscreen parameters: %*ph\n",
			NVT_TS_PARAMS_SIZE, data->buf);
		return -EIO;
	}

	dev_info(dev, "Detected %dx%d touchscreen with %d max touches\n",
		width, height, data->max_touches);

	if (data->buf[NVT_TS_PARAMS_MAX_BUTTONS])
		dev_warn(dev, "Touchscreen buttons are not supported\n");

	input = devm_input_allocate_device(dev);
	if (!input)
		return -ENOMEM;

	input->name = data->client->name;
	input->id.bustype = BUS_I2C;
	input->open = nvt_ts_start;
	input->close = nvt_ts_stop;

	input_set_abs_params(input, ABS_MT_POSITION_X, 0, width - 1, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, height - 1, 0, 0);
	touchscreen_parse_properties(input, true, &data->prop);

	error = input_mt_init_slots(input, data->max_touches,
				    INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
	if (error)
		return error;

	data->input = input;
	input_set_drvdata(input, data);

	error = devm_request_threaded_irq(dev, data->client->irq, NULL,
					  nvt_ts_irq,
					  IRQF_ONESHOT | IRQF_NO_AUTOEN |
						nvt_ts_irq_type[irq_type],
					  data->client->name, data);
	if (error) {
		dev_err(dev, "failed to request irq: %d\n", error);
		return error;
	}

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "failed to register input device: %d\n", error);
		return error;
	}

	return 0;
}

static int on_novatek_panel_prepared(struct drm_panel_follower *follower)
{
	struct nvt_ts_data *data = container_of(follower, struct nvt_ts_data, panel_follower);
	int ret;
	dev_info(&data->client->dev, "%s\n", __func__); // REMOVEME

	/* Is this the first power on? */
	if (!data->input) {
		dev_info(&data->client->dev, "doing initial power on\n"); // REMOVEME
		ret = nvt_ts_initial_power_on_and_register_inputdev(data);
		if (ret)
			return ret;
	}

	mutex_lock(&data->input->mutex);
	if (input_device_enabled(data->input))
		nvt_ts_start(data->input);
	mutex_unlock(&data->input->mutex);

	return 0;
}

static int on_novatek_panel_unpreparing(struct drm_panel_follower *follower)
{
	struct nvt_ts_data *data = container_of(follower, struct nvt_ts_data, panel_follower);
	dev_info(&data->client->dev, "%s\n", __func__); // REMOVEME

	if (!data->input)
		return 0;

	mutex_lock(&data->input->mutex);
	if (input_device_enabled(data->input))
		nvt_ts_stop(data->input);
	mutex_unlock(&data->input->mutex);

	return 0;
}

static const struct drm_panel_follower_funcs nvt_ts_follower_funcs = {
	.panel_prepared = on_novatek_panel_prepared,
	.panel_unpreparing = on_novatek_panel_unpreparing,
};

static DEFINE_SIMPLE_DEV_PM_OPS(nvt_ts_pm_ops, nvt_ts_suspend, nvt_ts_resume);

static int nvt_ts_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct nvt_ts_data *data;

	if (!client->irq) {
		dev_err(dev, "Error no irq specified\n");
		return -EINVAL;
	}

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	i2c_set_clientdata(client, data);

	/* check if "panel = <&...>" is set in DT */
	if (drm_is_panel_follower(dev)) {
		/* register self as follower */
		dev_info(dev, "probing in follower mode\n"); // REMOVEME
		data->is_panel_follower = true;
		data->panel_follower.funcs = &nvt_ts_follower_funcs;
		drm_panel_add_follower(dev, &data->panel_follower);
		/*
		 * In this mode, we can't do anything more at this moment.
		 * Need to wait for callbacks from panel.
		 */
		return 0;
	}

	dev_info(dev, "probing in normal mode\n"); // REMOVEME
	return nvt_ts_initial_power_on_and_register_inputdev(data);
}

static void nvt_ts_remove(struct i2c_client *client)
{
	struct nvt_ts_data *data = i2c_get_clientdata(client);

	if (data->is_panel_follower)
		drm_panel_remove_follower(&data->panel_follower);
}

static const struct nvt_ts_i2c_chip_data nvt_nt11205_ts_data = {
	.wake_type = 0x05,
	.chip_id = 0x05,
};

static const struct nvt_ts_i2c_chip_data nvt_nt36672a_ts_data = {
	.wake_type = 0x01,
	.chip_id = 0x08,
};

static const struct of_device_id nvt_ts_of_match[] = {
	{ .compatible = "novatek,nt11205-ts", .data = &nvt_nt11205_ts_data },
	{ .compatible = "novatek,nt36672a-ts", .data = &nvt_nt36672a_ts_data },
	{ }
};
MODULE_DEVICE_TABLE(of, nvt_ts_of_match);

static const struct i2c_device_id nvt_ts_i2c_id[] = {
	{ "nt11205-ts", (unsigned long) &nvt_nt11205_ts_data },
	{ "nt36672a-ts", (unsigned long) &nvt_nt36672a_ts_data },
	{ }
};
MODULE_DEVICE_TABLE(i2c, nvt_ts_i2c_id);

static struct i2c_driver nvt_ts_driver = {
	.driver = {
		.name	= "novatek-nvt-ts",
		.pm	= pm_sleep_ptr(&nvt_ts_pm_ops),
		.of_match_table = nvt_ts_of_match,
	},
	.probe = nvt_ts_probe,
	.remove = nvt_ts_remove,
	.id_table = nvt_ts_i2c_id,
};

module_i2c_driver(nvt_ts_driver);

MODULE_DESCRIPTION("Novatek NT11205 touchscreen driver");
MODULE_AUTHOR("Hans de Goede <hdegoede@redhat.com>");
MODULE_LICENSE("GPL");

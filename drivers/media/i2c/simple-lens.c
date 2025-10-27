// SPDX-License-Identifier: GPL-2.0

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/time.h>
#include <linux/regulator/consumer.h>

#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

struct simple_lens_data {
	const char *name;
	u16 pos_reg, max_pos;

	const struct reg_sequence *on_seq;
	int on_seq_len;
	const struct reg_sequence *off_seq;
	int off_seq_len;
};

struct simple_lens {
	struct device *dev;
	const struct simple_lens_data *data;
	struct regmap *rmap;
	struct regulator_bulk_data *supplies;
	int num_supplies;
	struct v4l2_ctrl *focus;
	struct v4l2_ctrl_handler chdl;
	struct v4l2_subdev sd;
};

#define to_lens(ptr, member) container_of(ptr, struct simple_lens, member)

struct reg_sequence zc533_power_on[] ={
	{ 0x02, 0x00 },
	{ 0x05, 0x05 },
	{ 0x06, 0x64 },
	{ 0x07, 0x64 },
	{ 0x08, 0x03 },
};

struct reg_sequence zc533_power_off[] ={
	{ 0x02, 0x01 },
};

static int poll_reg = 0;
module_param(poll_reg, int, 0644);

static int simple_lense_set_position_be16(struct simple_lens *lens, u16 pos)
{
	u16 regval = ((pos & 0xff) << 8) | (pos >> 8);

	return regmap_bulk_write(lens->rmap, lens->data->pos_reg, &regval, 2);
}

struct simple_lens_data zc533_data = {
	.name = "zc533",
	.pos_reg = 0x03,
	.max_pos = 0x3ff,
	.on_seq = zc533_power_on,
	.on_seq_len = ARRAY_SIZE(zc533_power_on),
	.off_seq = zc533_power_off,
	.off_seq_len = ARRAY_SIZE(zc533_power_off),
};

static int simple_lens_runtime_suspend(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct simple_lens *lens = to_lens(sd, sd);

	regmap_multi_reg_write(lens->rmap,
			lens->data->off_seq,
			lens->data->off_seq_len);

	return regulator_bulk_disable(lens->num_supplies, lens->supplies);
}

static int simple_lens_power_on(struct simple_lens *lens, bool init)
{
	int ret;

	ret = regulator_bulk_enable(lens->num_supplies, lens->supplies);
	if (ret) {
		dev_err(lens->dev, "failed to enable regulators\n");
		return ret;
	}

	if (!init)
		return 0;

	msleep(10);

	return regmap_multi_reg_write(lens->rmap,
			lens->data->on_seq,
			lens->data->on_seq_len);
}

static int simple_lens_runtime_resume(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct simple_lens *lens = to_lens(sd, sd);

	return simple_lens_power_on(lens, true);
}

static int simple_lens_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct simple_lens *lens = to_lens(ctrl->handler, chdl);

	if (ctrl->id != V4L2_CID_FOCUS_ABSOLUTE)
		return 0;

	return simple_lense_set_position_be16(lens, ctrl->val);
}

static const struct v4l2_ctrl_ops simple_lens_ctrl_ops = {
	.s_ctrl = simple_lens_set_ctrl,
};

static int simple_lens_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	return pm_runtime_resume_and_get(sd->dev);
}

static int simple_lens_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	pm_runtime_put_autosuspend(sd->dev);
	return 0;
}

static const struct v4l2_subdev_internal_ops lens_int_ops = {
	.open = simple_lens_open,
	.close = simple_lens_close,
};

static const struct v4l2_subdev_ops dummy_ops = { };

static void lens_free_regulators(void *param)
{
	struct simple_lens *lens = param;
	regulator_bulk_free(lens->num_supplies, lens->supplies);
}

static const struct regmap_config regmap_config_8b = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x1f,
	.cache_type = REGCACHE_NONE,
};

static int lens_probe(struct i2c_client *client)
{
	const struct v4l2_ctrl_ops *ops = &simple_lens_ctrl_ops;
	const struct simple_lens_data *data;
	struct device *dev = &client->dev;
	struct simple_lens *lens;
	struct v4l2_ctrl_handler *hdl;
	int ret, tries = 5;
	u32 value = 0;

	lens = devm_kzalloc(dev, sizeof(*lens), GFP_KERNEL);
	if (!lens)
		return -ENOMEM;

	lens->data = data = of_device_get_match_data(dev);
	lens->rmap = devm_regmap_init_i2c(client, &regmap_config_8b);
	ret = PTR_ERR_OR_ZERO(lens->rmap);
	if (ret)
		return dev_err_probe(dev, ret, "failed to init regmap\n");

	ret = of_regulator_bulk_get_all(dev, dev->of_node, &lens->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to get regulators\n");

	lens->num_supplies = ret;
	lens->dev = dev;

	ret = devm_add_action_or_reset(dev, lens_free_regulators, lens);
	if (ret)
		return ret;

	ret = regulator_bulk_enable(lens->num_supplies, lens->supplies);
	if (ret)
		return ret;

	do {
		msleep(100);
		ret = regmap_read(lens->rmap, 0, &value);
	} while (ret < 0 && tries -- > 0);

	if (ret)
		dev_err_probe(dev, ret, "failed to read device");

	regulator_bulk_disable(lens->num_supplies, lens->supplies);
	if (ret)
		return ret;

	/* Initialize subdev */
	v4l2_i2c_subdev_init(&lens->sd, client, &dummy_ops);

	hdl = &lens->chdl;
	v4l2_ctrl_handler_init(hdl, 1);
	v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE, 0,
			lens->data->max_pos, 1, 0);

	ret = hdl->error;
	if (ret) {
		dev_err_probe(dev, ret, "failed to init controls\n");
		goto err_handler_cleanup;
	}

	snprintf(lens->sd.name, sizeof(lens->sd.name),
		 "%s %s", data->name, dev_name(dev));

	lens->sd.ctrl_handler = hdl;
	lens->sd.entity.function = MEDIA_ENT_F_LENS;
	lens->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	lens->sd.internal_ops = &lens_int_ops;

	ret = media_entity_pads_init(&lens->sd.entity, 0, NULL);
	if (ret < 0) {
		dev_err_probe(dev, ret, "failed to init pads\n");
		goto err_handler_cleanup;
	}

	ret = v4l2_async_register_subdev(&lens->sd);
	if (ret < 0) {
		dev_err_probe(dev, ret, "failed to register subdev\n");
		goto err_entity_cleanup;
	}

	// FIXME: this wouldn't work kernel without CONFIG_PM
	pm_runtime_set_suspended(dev);
	pm_runtime_enable(dev);
	pm_runtime_set_autosuspend_delay(dev, 2000);
	pm_runtime_use_autosuspend(dev);
	return 0;

err_entity_cleanup:
	media_entity_cleanup(&lens->sd.entity);
err_handler_cleanup:
	v4l2_ctrl_handler_free(&lens->chdl);
	v4l2_subdev_cleanup(&lens->sd);
	return ret;

}

static void lens_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct device *dev = &client->dev;
	struct simple_lens *lens = to_lens(sd, sd);

	v4l2_async_unregister_subdev(sd);
	v4l2_subdev_cleanup(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(&lens->chdl);

	pm_runtime_disable(dev);
	if (!pm_runtime_status_suspended(dev))
		simple_lens_runtime_suspend(dev);
	pm_runtime_set_suspended(dev);
	pm_runtime_dont_use_autosuspend(dev);
}

static const struct of_device_id lens_of_table[] = {
	{ .compatible = "powerlogics,zc535", &zc533_data },
	{}
};
MODULE_DEVICE_TABLE(of, lens_of_table);

static const struct dev_pm_ops lens_pm_ops = {
	SET_RUNTIME_PM_OPS(simple_lens_runtime_suspend,
			   simple_lens_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

static struct i2c_driver lens_i2c_driver = {
	.driver = {
		.name = "simple-lens",
		.pm = &lens_pm_ops,
		.of_match_table = lens_of_table,
	},
	.probe = lens_probe,
	.remove = lens_remove,
};
module_i2c_driver(lens_i2c_driver);

MODULE_AUTHOR("Vladimir Lypak <vladimir.lypak@gmail.com>");
MODULE_DESCRIPTION("I2C lens actuator driver");
MODULE_LICENSE("GPL v2");

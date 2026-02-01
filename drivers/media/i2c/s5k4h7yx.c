// SPDX-License-Identifier: GPL-2.0-only
/*
 * A V4L2 driver for Samsung S5K4H7YX cameras.
 * Copyright (C) 2024 Luca Weiss <luca.weiss@fairphone.com>
 *
 * Based on Sony imx412 camera driver
 * Copyright (C) 2021 Intel Corporation
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include <media/v4l2-cci.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

/* Chip ID */
#define S5K4H7YX_REG_CHIP_ID		CCI_REG16(0x0000)
#define S5K4H7YX_CHIP_ID			0x487b

/* Streaming Mode */
#define S5K4H7YX_REG_MODE_SELECT		CCI_REG8(0x0100)
#define S5K4H7YX_MODE_STANDBY		0x00
#define S5K4H7YX_MODE_STREAMING		0x01

/* Lines per frame */
#define S5K4H7YX_REG_LPFR			CCI_REG16(0x0340)

/* Exposure control */
#define S5K4H7YX_REG_EXPOSURE		CCI_REG16(0x0202)
#define S5K4H7YX_EXPOSURE_MIN		8
#define S5K4H7YX_EXPOSURE_OFFSET		22
#define S5K4H7YX_EXPOSURE_STEP		1
#define S5K4H7YX_EXPOSURE_DEFAULT		0x0648

/* Analog gain control */
#define S5K4H7YX_REG_ANALOG_GAIN		CCI_REG16(0x0204)
#define S5K4H7YX_ANA_GAIN_MIN		0
#define S5K4H7YX_ANA_GAIN_MAX		978
#define S5K4H7YX_ANA_GAIN_STEP		1
#define S5K4H7YX_ANA_GAIN_DEFAULT		0

/* Group hold register */
#define S5K4H7YX_REG_HOLD		CCI_REG8(0x0104)

/* Input clock rate */
#define S5K4H7YX_INCLK_RATE	24000000

/* CSI2 HW configuration */
#define S5K4H7YX_LINK_FREQ	600000000
#define S5K4H7YX_NUM_DATA_LANES	4

#define S5K4H7YX_REG_MIN		0x00
#define S5K4H7YX_REG_MAX		0xffff

/**
 * struct s5k4h7yx_reg_list - s5k4h7yx sensor register list
 * @num_of_regs: Number of registers in the list
 * @regs: Pointer to register list
 */
struct s5k4h7yx_reg_list {
	u32 num_of_regs;
	const struct cci_reg_sequence *regs;
};

/**
 * struct s5k4h7yx_mode - s5k4h7yx sensor mode structure
 * @width: Frame width
 * @height: Frame height
 * @code: Format code
 * @hblank: Horizontal blanking in lines
 * @vblank: Vertical blanking in lines
 * @vblank_min: Minimum vertical blanking in lines
 * @vblank_max: Maximum vertical blanking in lines
 * @pclk: Sensor pixel clock
 * @link_freq_idx: Link frequency index
 * @reg_list: Register list for sensor mode
 */
struct s5k4h7yx_mode {
	u32 width;
	u32 height;
	u32 code;
	u32 hblank;
	u32 vblank;
	u32 vblank_min;
	u32 vblank_max;
	u64 pclk;
	u32 link_freq_idx;
	struct s5k4h7yx_reg_list reg_list;
};

static const char * const s5k4h7yx_supply_names[] = {
	"vdda",		/* 2.8V Analog Power */
	"vddio",	/* 1.8V Interface Power */
	"vddd",		/* 1.2V Digital Power */
};

/**
 * struct s5k4h7yx - s5k4h7yx sensor device structure
 * @dev: Pointer to generic device
 * @client: Pointer to i2c client
 * @sd: V4L2 sub-device
 * @pad: Media pad. Only one pad supported
 * @reset_gpio: Sensor reset gpio
 * @inclk: Sensor input clock
 * @supplies: Regulator supplies
 * @ctrl_handler: V4L2 control handler
 * @link_freq_ctrl: Pointer to link frequency control
 * @pclk_ctrl: Pointer to pixel clock control
 * @hblank_ctrl: Pointer to horizontal blanking control
 * @vblank_ctrl: Pointer to vertical blanking control
 * @exp_ctrl: Pointer to exposure control
 * @again_ctrl: Pointer to analog gain control
 * @vblank: Vertical blanking in lines
 * @cur_mode: Pointer to current selected sensor mode
 * @mutex: Mutex for serializing sensor controls
 */
struct s5k4h7yx {
	struct device *dev;
	struct i2c_client *client;
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct gpio_desc *reset_gpio;
	struct clk *inclk;
	struct regulator_bulk_data supplies[ARRAY_SIZE(s5k4h7yx_supply_names)];
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl *link_freq_ctrl;
	struct v4l2_ctrl *pclk_ctrl;
	struct v4l2_ctrl *hblank_ctrl;
	struct v4l2_ctrl *vblank_ctrl;
	struct {
		struct v4l2_ctrl *exp_ctrl;
		struct v4l2_ctrl *again_ctrl;
	};
	u32 vblank;
	const struct s5k4h7yx_mode *cur_mode;
	struct mutex mutex;
	struct regmap *regmap;
};

static const s64 link_freq[] = {
	S5K4H7YX_LINK_FREQ,
};

/* Sensor mode registers */
static const struct cci_reg_sequence mode_3264x2448_regs[] = {
	// common registers
	{ CCI_REG8(0x0100), 0x00 },
	{ CCI_REG8(0x0B05), 0x01 },
	{ CCI_REG8(0x3074), 0x06 },
	{ CCI_REG8(0x3075), 0x2F },
	{ CCI_REG8(0x308A), 0x20 },
	{ CCI_REG8(0x308B), 0x08 },
	{ CCI_REG8(0x308C), 0x0B },
	{ CCI_REG8(0x3081), 0x07 },
	{ CCI_REG8(0x307B), 0x85 },
	{ CCI_REG8(0x307A), 0x0A },
	{ CCI_REG8(0x3079), 0x0A },
	{ CCI_REG8(0x306E), 0x71 },
	{ CCI_REG8(0x306F), 0x28 },
	{ CCI_REG8(0x301F), 0x20 },
	{ CCI_REG8(0x306B), 0x9A },
	{ CCI_REG8(0x3091), 0x1F },
	{ CCI_REG8(0x30C4), 0x06 },
	{ CCI_REG8(0x3200), 0x09 },
	{ CCI_REG8(0x306A), 0x79 },
	{ CCI_REG8(0x30B0), 0xFF },
	{ CCI_REG8(0x306D), 0x08 },
	{ CCI_REG8(0x3080), 0x00 },
	{ CCI_REG8(0x3929), 0x3F },
	{ CCI_REG8(0x3084), 0x16 },
	{ CCI_REG8(0x3070), 0x0F },
	{ CCI_REG8(0x3B45), 0x01 },
	{ CCI_REG8(0x30C2), 0x05 },
	{ CCI_REG8(0x3069), 0x87 },
	{ CCI_REG8(0x3924), 0x7F },
	{ CCI_REG8(0x3925), 0xFD },
	{ CCI_REG8(0x3C08), 0xFF },
	{ CCI_REG8(0x3C09), 0xFF },
	{ CCI_REG8(0x3C31), 0xFF },
	{ CCI_REG8(0x3C32), 0xFF },
	{ CCI_REG8(0x0A02), 0x14 },

	// Res 0
	{ CCI_REG8(0x0100), 0x00 },
	{ CCI_REG8(0x0136), 0x18 },
	{ CCI_REG8(0x0137), 0x00 },
	{ CCI_REG8(0x0305), 0x06 },
	{ CCI_REG8(0x0306), 0x00 },
	{ CCI_REG8(0x0307), 0x8C },
	{ CCI_REG8(0x030D), 0x06 },
	{ CCI_REG8(0x030E), 0x00 },
	{ CCI_REG8(0x030F), 0xAF },
	{ CCI_REG8(0x3C1F), 0x00 },
	{ CCI_REG8(0x3C17), 0x00 },
	{ CCI_REG8(0x3C1C), 0x05 },
	{ CCI_REG8(0x3C1D), 0x15 },
	{ CCI_REG8(0x0301), 0x04 },
	{ CCI_REG8(0x0820), 0x02 },
	{ CCI_REG8(0x0821), 0xBC },
	{ CCI_REG8(0x0822), 0x00 },
	{ CCI_REG8(0x0823), 0x00 },
	{ CCI_REG8(0x0112), 0x0A },
	{ CCI_REG8(0x0113), 0x0A },
	{ CCI_REG8(0x0114), 0x03 },
	{ CCI_REG8(0x3906), 0x04 },
	{ CCI_REG8(0x0344), 0x00 },
	{ CCI_REG8(0x0345), 0x08 },
	{ CCI_REG8(0x0346), 0x00 },
	{ CCI_REG8(0x0347), 0x08 },
	{ CCI_REG8(0x0348), 0x0C },
	{ CCI_REG8(0x0349), 0xC7 },
	{ CCI_REG8(0x034A), 0x09 },
	{ CCI_REG8(0x034B), 0x97 },
	{ CCI_REG8(0x034C), 0x0C },
	{ CCI_REG8(0x034D), 0xC0 },
	{ CCI_REG8(0x034E), 0x09 },
	{ CCI_REG8(0x034F), 0x90 },
	{ CCI_REG8(0x0900), 0x00 },
	{ CCI_REG8(0x0901), 0x00 },
	{ CCI_REG8(0x0381), 0x01 },
	{ CCI_REG8(0x0383), 0x01 },
	{ CCI_REG8(0x0385), 0x01 },
	{ CCI_REG8(0x0387), 0x01 },
	{ CCI_REG8(0x0101), 0x00 },
	{ CCI_REG8(0x0340), 0x09 },
	{ CCI_REG8(0x0341), 0xE2 },
	{ CCI_REG8(0x0342), 0x0E },
	{ CCI_REG8(0x0343), 0x68 },
	{ CCI_REG8(0x0200), 0x0D },
	{ CCI_REG8(0x0201), 0xD8 },
	{ CCI_REG8(0x0202), 0x02 },
	{ CCI_REG8(0x0203), 0x08 },
};

/* Supported sensor mode configurations */
static const struct s5k4h7yx_mode supported_mode = {
	.width = 3264,
	.height = 2448,
	.hblank = 456, // FIXME
	.vblank = 506, // FIXME
	.vblank_min = 506, // FIXME
	.vblank_max = 32420, // FIXME
	.pclk = 619200000, // outputPixelClock?
	.link_freq_idx = 0,
	.code = MEDIA_BUS_FMT_SGRBG10_1X10,
	.reg_list = {
		.num_of_regs = ARRAY_SIZE(mode_3264x2448_regs),
		.regs = mode_3264x2448_regs,
	},
};

/**
 * to_s5k4h7yx() - s5k4h7yx V4L2 sub-device to s5k4h7yx device.
 * @subdev: pointer to s5k4h7yx V4L2 sub-device
 *
 * Return: pointer to s5k4h7yx device
 */
static inline struct s5k4h7yx *to_s5k4h7yx(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct s5k4h7yx, sd);
}

/**
 * s5k4h7yx_update_controls() - Update control ranges based on streaming mode
 * @s5k4h7yx: pointer to s5k4h7yx device
 * @mode: pointer to s5k4h7yx_mode sensor mode
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_update_controls(struct s5k4h7yx *s5k4h7yx,
				  const struct s5k4h7yx_mode *mode)
{
	int ret;

	ret = __v4l2_ctrl_s_ctrl(s5k4h7yx->link_freq_ctrl, mode->link_freq_idx);
	if (ret)
		return ret;

	ret = __v4l2_ctrl_s_ctrl(s5k4h7yx->hblank_ctrl, mode->hblank);
	if (ret)
		return ret;

	return __v4l2_ctrl_modify_range(s5k4h7yx->vblank_ctrl, mode->vblank_min,
					mode->vblank_max, 1, mode->vblank);
}

/**
 * s5k4h7yx_update_exp_gain() - Set updated exposure and gain
 * @s5k4h7yx: pointer to s5k4h7yx device
 * @exposure: updated exposure value
 * @gain: updated analog gain value
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_update_exp_gain(struct s5k4h7yx *s5k4h7yx, u32 exposure, u32 gain)
{
	u32 lpfr;
	int ret = 0;

	lpfr = s5k4h7yx->vblank + s5k4h7yx->cur_mode->height;

	dev_dbg(s5k4h7yx->dev, "Set exp %u, analog gain %u, lpfr %u\n",
		exposure, gain, lpfr);

	cci_write(s5k4h7yx->regmap, S5K4H7YX_REG_HOLD, 1, &ret);
	if (ret)
		return ret;

	cci_write(s5k4h7yx->regmap, S5K4H7YX_REG_LPFR, lpfr, &ret);
	if (ret)
		goto error_release_group_hold;

	cci_write(s5k4h7yx->regmap, S5K4H7YX_REG_EXPOSURE, exposure, &ret);
	if (ret)
		goto error_release_group_hold;

	cci_write(s5k4h7yx->regmap, S5K4H7YX_REG_ANALOG_GAIN, gain, &ret);

error_release_group_hold:
	cci_write(s5k4h7yx->regmap, S5K4H7YX_REG_HOLD, 0, NULL);

	return ret;
}

/**
 * s5k4h7yx_set_ctrl() - Set subdevice control
 * @ctrl: pointer to v4l2_ctrl structure
 *
 * Supported controls:
 * - V4L2_CID_VBLANK
 * - cluster controls:
 *   - V4L2_CID_ANALOGUE_GAIN
 *   - V4L2_CID_EXPOSURE
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct s5k4h7yx *s5k4h7yx =
		container_of(ctrl->handler, struct s5k4h7yx, ctrl_handler);
	u32 analog_gain;
	u32 exposure;
	int ret;

	switch (ctrl->id) {
	case V4L2_CID_VBLANK:
		s5k4h7yx->vblank = s5k4h7yx->vblank_ctrl->val;

		dev_dbg(s5k4h7yx->dev, "Received vblank %u, new lpfr %u\n",
			s5k4h7yx->vblank,
			s5k4h7yx->vblank + s5k4h7yx->cur_mode->height);

		ret = __v4l2_ctrl_modify_range(s5k4h7yx->exp_ctrl,
					       S5K4H7YX_EXPOSURE_MIN,
					       s5k4h7yx->vblank +
					       s5k4h7yx->cur_mode->height -
					       S5K4H7YX_EXPOSURE_OFFSET,
					       1, S5K4H7YX_EXPOSURE_DEFAULT);
		break;
	case V4L2_CID_EXPOSURE:
		/* Set controls only if sensor is in power on state */
		if (!pm_runtime_get_if_in_use(s5k4h7yx->dev))
			return 0;

		exposure = ctrl->val;
		analog_gain = s5k4h7yx->again_ctrl->val;

		dev_dbg(s5k4h7yx->dev, "Received exp %u, analog gain %u\n",
			exposure, analog_gain);

		ret = s5k4h7yx_update_exp_gain(s5k4h7yx, exposure, analog_gain);

		pm_runtime_put(s5k4h7yx->dev);

		break;
	default:
		dev_err(s5k4h7yx->dev, "Invalid control %d\n", ctrl->id);
		ret = -EINVAL;
	}

	return ret;
}

/* V4l2 subdevice control ops*/
static const struct v4l2_ctrl_ops s5k4h7yx_ctrl_ops = {
	.s_ctrl = s5k4h7yx_set_ctrl,
};

/**
 * s5k4h7yx_enum_mbus_code() - Enumerate V4L2 sub-device mbus codes
 * @sd: pointer to s5k4h7yx V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @code: V4L2 sub-device code enumeration need to be filled
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index > 0)
		return -EINVAL;

	code->code = supported_mode.code;

	return 0;
}

/**
 * s5k4h7yx_enum_frame_size() - Enumerate V4L2 sub-device frame sizes
 * @sd: pointer to s5k4h7yx V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @fsize: V4L2 sub-device size enumeration need to be filled
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_frame_size_enum *fsize)
{
	if (fsize->index > 0)
		return -EINVAL;

	if (fsize->code != supported_mode.code)
		return -EINVAL;

	fsize->min_width = supported_mode.width;
	fsize->max_width = fsize->min_width;
	fsize->min_height = supported_mode.height;
	fsize->max_height = fsize->min_height;

	return 0;
}

/**
 * s5k4h7yx_fill_pad_format() - Fill subdevice pad format
 *                            from selected sensor mode
 * @s5k4h7yx: pointer to s5k4h7yx device
 * @mode: pointer to s5k4h7yx_mode sensor mode
 * @fmt: V4L2 sub-device format need to be filled
 */
static void s5k4h7yx_fill_pad_format(struct s5k4h7yx *s5k4h7yx,
				   const struct s5k4h7yx_mode *mode,
				   struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = mode->code;
	fmt->format.field = V4L2_FIELD_NONE;
	fmt->format.colorspace = V4L2_COLORSPACE_RAW;
	fmt->format.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	fmt->format.quantization = V4L2_QUANTIZATION_DEFAULT;
	fmt->format.xfer_func = V4L2_XFER_FUNC_NONE;
}

/**
 * s5k4h7yx_get_pad_format() - Get subdevice pad format
 * @sd: pointer to s5k4h7yx V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @fmt: V4L2 sub-device format need to be set
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_get_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_format *fmt)
{
	struct s5k4h7yx *s5k4h7yx = to_s5k4h7yx(sd);

	mutex_lock(&s5k4h7yx->mutex);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		struct v4l2_mbus_framefmt *framefmt;

		framefmt = v4l2_subdev_state_get_format(sd_state, fmt->pad);
		fmt->format = *framefmt;
	} else {
		s5k4h7yx_fill_pad_format(s5k4h7yx, s5k4h7yx->cur_mode, fmt);
	}

	mutex_unlock(&s5k4h7yx->mutex);

	return 0;
}

/**
 * s5k4h7yx_set_pad_format() - Set subdevice pad format
 * @sd: pointer to s5k4h7yx V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @fmt: V4L2 sub-device format need to be set
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_set_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_format *fmt)
{
	struct s5k4h7yx *s5k4h7yx = to_s5k4h7yx(sd);
	const struct s5k4h7yx_mode *mode;
	int ret = 0;

	mutex_lock(&s5k4h7yx->mutex);

	mode = &supported_mode;
	s5k4h7yx_fill_pad_format(s5k4h7yx, mode, fmt);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		struct v4l2_mbus_framefmt *framefmt;

		framefmt = v4l2_subdev_state_get_format(sd_state, fmt->pad);
		*framefmt = fmt->format;
	} else {
		ret = s5k4h7yx_update_controls(s5k4h7yx, mode);
		if (!ret)
			s5k4h7yx->cur_mode = mode;
	}

	mutex_unlock(&s5k4h7yx->mutex);

	return ret;
}

/**
 * s5k4h7yx_init_state() - Initialize sub-device state
 * @sd: pointer to s5k4h7yx V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_init_state(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state)
{
	struct s5k4h7yx *s5k4h7yx = to_s5k4h7yx(sd);
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = sd_state ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	s5k4h7yx_fill_pad_format(s5k4h7yx, &supported_mode, &fmt);

	return s5k4h7yx_set_pad_format(sd, sd_state, &fmt);
}

/**
 * s5k4h7yx_start_streaming() - Start sensor stream
 * @s5k4h7yx: pointer to s5k4h7yx device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_start_streaming(struct s5k4h7yx *s5k4h7yx)
{
	const struct s5k4h7yx_reg_list *reg_list;
	int ret;

	/* Write sensor mode registers */
	reg_list = &s5k4h7yx->cur_mode->reg_list;
	ret = cci_multi_reg_write(s5k4h7yx->regmap, reg_list->regs,
				  reg_list->num_of_regs, NULL);
	if (ret) {
		dev_err(s5k4h7yx->dev, "fail to write initial registers\n");
		return ret;
	}

	/* Setup handler will write actual exposure and gain */
	ret =  __v4l2_ctrl_handler_setup(s5k4h7yx->sd.ctrl_handler);
	if (ret) {
		dev_err(s5k4h7yx->dev, "fail to setup handler\n");
		return ret;
	}

	/* Delay is required before streaming*/
	usleep_range(7400, 8000);

	/* Start streaming */
	cci_write(s5k4h7yx->regmap, S5K4H7YX_REG_MODE_SELECT, S5K4H7YX_MODE_STREAMING, &ret);
	if (ret) {
		dev_err(s5k4h7yx->dev, "fail to start streaming\n");
		return ret;
	}

	return 0;
}

/**
 * s5k4h7yx_stop_streaming() - Stop sensor stream
 * @s5k4h7yx: pointer to s5k4h7yx device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_stop_streaming(struct s5k4h7yx *s5k4h7yx)
{
	return cci_write(s5k4h7yx->regmap, S5K4H7YX_REG_MODE_SELECT,
			 S5K4H7YX_MODE_STANDBY, NULL);
}

/**
 * s5k4h7yx_set_stream() - Enable sensor streaming
 * @sd: pointer to s5k4h7yx subdevice
 * @enable: set to enable sensor streaming
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct s5k4h7yx *s5k4h7yx = to_s5k4h7yx(sd);
	int ret;

	mutex_lock(&s5k4h7yx->mutex);

	if (enable) {
		ret = pm_runtime_resume_and_get(s5k4h7yx->dev);
		if (ret)
			goto error_unlock;

		ret = s5k4h7yx_start_streaming(s5k4h7yx);
		if (ret)
			goto error_power_off;
	} else {
		s5k4h7yx_stop_streaming(s5k4h7yx);
		pm_runtime_put(s5k4h7yx->dev);
	}

	mutex_unlock(&s5k4h7yx->mutex);

	return 0;

error_power_off:
	pm_runtime_put(s5k4h7yx->dev);
error_unlock:
	mutex_unlock(&s5k4h7yx->mutex);

	return ret;
}

/**
 * s5k4h7yx_detect() - Detect s5k4h7yx sensor
 * @s5k4h7yx: pointer to s5k4h7yx device
 *
 * Return: 0 if successful, -EIO if sensor id does not match
 */
static int s5k4h7yx_detect(struct s5k4h7yx *s5k4h7yx)
{
	int ret;
	u64 val;

	ret = cci_read(s5k4h7yx->regmap, S5K4H7YX_REG_CHIP_ID, &val, NULL);
	if (ret)
		return ret;

	if (val != S5K4H7YX_CHIP_ID) {
		dev_err(s5k4h7yx->dev, "chip id mismatch: %x!=%llx\n",
			S5K4H7YX_CHIP_ID, val);
		return -ENXIO;
	}

	return 0;
}

/**
 * s5k4h7yx_parse_hw_config() - Parse HW configuration and check if supported
 * @s5k4h7yx: pointer to s5k4h7yx device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_parse_hw_config(struct s5k4h7yx *s5k4h7yx)
{
	struct fwnode_handle *fwnode = dev_fwnode(s5k4h7yx->dev);
	struct v4l2_fwnode_endpoint bus_cfg = {};
	struct fwnode_handle *ep;
	unsigned long rate;
	unsigned int i;
	int ret;

	if (!fwnode)
		return -ENXIO;

	/* Request optional reset pin */
	s5k4h7yx->reset_gpio = devm_gpiod_get_optional(s5k4h7yx->dev, "reset",
						     GPIOD_OUT_LOW);
	if (IS_ERR(s5k4h7yx->reset_gpio)) {
		dev_err(s5k4h7yx->dev, "failed to get reset gpio %ld\n",
			PTR_ERR(s5k4h7yx->reset_gpio));
		return PTR_ERR(s5k4h7yx->reset_gpio);
	}

	/* Get sensor input clock */
	s5k4h7yx->inclk = devm_clk_get(s5k4h7yx->dev, NULL);
	if (IS_ERR(s5k4h7yx->inclk)) {
		dev_err(s5k4h7yx->dev, "could not get inclk\n");
		return PTR_ERR(s5k4h7yx->inclk);
	}

	rate = clk_get_rate(s5k4h7yx->inclk);
	if (rate != S5K4H7YX_INCLK_RATE) {
		dev_err(s5k4h7yx->dev, "inclk frequency mismatch\n");
		return -EINVAL;
	}

	/* Get optional DT defined regulators */
	for (i = 0; i < ARRAY_SIZE(s5k4h7yx_supply_names); i++)
		s5k4h7yx->supplies[i].supply = s5k4h7yx_supply_names[i];

	ret = devm_regulator_bulk_get(s5k4h7yx->dev,
				      ARRAY_SIZE(s5k4h7yx_supply_names),
				      s5k4h7yx->supplies);
	if (ret)
		return ret;

	ep = fwnode_graph_get_next_endpoint(fwnode, NULL);
	if (!ep)
		return -ENXIO;

	ret = v4l2_fwnode_endpoint_alloc_parse(ep, &bus_cfg);
	fwnode_handle_put(ep);
	if (ret)
		return ret;

	if (bus_cfg.bus_type != V4L2_MBUS_CSI2_DPHY) {
		dev_err(s5k4h7yx->dev, "selected bus-type is not supported\n");
		ret = -EINVAL;
		goto done_endpoint_free;
	}

	if (bus_cfg.bus.mipi_csi2.num_data_lanes != S5K4H7YX_NUM_DATA_LANES) {
		dev_err(s5k4h7yx->dev,
			"number of CSI2 data lanes %d is not supported\n",
			bus_cfg.bus.mipi_csi2.num_data_lanes);
		ret = -EINVAL;
		goto done_endpoint_free;
	}

	if (!bus_cfg.nr_of_link_frequencies) {
		dev_err(s5k4h7yx->dev, "no link frequencies defined\n");
		ret = -EINVAL;
		goto done_endpoint_free;
	}

	for (i = 0; i < bus_cfg.nr_of_link_frequencies; i++)
		if (bus_cfg.link_frequencies[i] == S5K4H7YX_LINK_FREQ)
			goto done_endpoint_free;

	ret = -EINVAL;

done_endpoint_free:
	v4l2_fwnode_endpoint_free(&bus_cfg);

	return ret;
}

/* V4l2 subdevice ops */
static const struct v4l2_subdev_video_ops s5k4h7yx_video_ops = {
	.s_stream = s5k4h7yx_set_stream,
};

static const struct v4l2_subdev_pad_ops s5k4h7yx_pad_ops = {
	.enum_mbus_code = s5k4h7yx_enum_mbus_code,
	.enum_frame_size = s5k4h7yx_enum_frame_size,
	.get_fmt = s5k4h7yx_get_pad_format,
	.set_fmt = s5k4h7yx_set_pad_format,
};

static const struct v4l2_subdev_ops s5k4h7yx_subdev_ops = {
	.video = &s5k4h7yx_video_ops,
	.pad = &s5k4h7yx_pad_ops,
};

static const struct v4l2_subdev_internal_ops s5k4h7yx_internal_ops = {
	.init_state = s5k4h7yx_init_state,
};

/**
 * s5k4h7yx_power_on() - Sensor power on sequence
 * @dev: pointer to i2c device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_power_on(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct s5k4h7yx *s5k4h7yx = to_s5k4h7yx(sd);
	int ret;

	gpiod_set_value_cansleep(s5k4h7yx->reset_gpio, 1);

	usleep_range(10000, 10100);

	ret = regulator_bulk_enable(ARRAY_SIZE(s5k4h7yx_supply_names),
				    s5k4h7yx->supplies);
	if (ret < 0) {
		dev_err(dev, "failed to enable regulators\n");
		return ret;
	}

	usleep_range(10000, 10100);

	gpiod_set_value_cansleep(s5k4h7yx->reset_gpio, 0);

	usleep_range(8000, 8100);

	ret = clk_prepare_enable(s5k4h7yx->inclk);
	if (ret) {
		dev_err(s5k4h7yx->dev, "fail to enable inclk\n");
		goto error_reset;
	}

	usleep_range(15000, 15100);

	return 0;

error_reset:
	gpiod_set_value_cansleep(s5k4h7yx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(s5k4h7yx_supply_names),
			       s5k4h7yx->supplies);

	return ret;
}

/**
 * s5k4h7yx_power_off() - Sensor power off sequence
 * @dev: pointer to i2c device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct s5k4h7yx *s5k4h7yx = to_s5k4h7yx(sd);

	clk_disable_unprepare(s5k4h7yx->inclk);

	gpiod_set_value_cansleep(s5k4h7yx->reset_gpio, 1);

	regulator_bulk_disable(ARRAY_SIZE(s5k4h7yx_supply_names),
			       s5k4h7yx->supplies);

	return 0;
}

/**
 * s5k4h7yx_init_controls() - Initialize sensor subdevice controls
 * @s5k4h7yx: pointer to s5k4h7yx device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_init_controls(struct s5k4h7yx *s5k4h7yx)
{
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl_handler *ctrl_hdlr = &s5k4h7yx->ctrl_handler;
	const struct s5k4h7yx_mode *mode = s5k4h7yx->cur_mode;
	u32 lpfr;
	int ret;

	/* set properties from fwnode (e.g. rotation, orientation) */
	ret = v4l2_fwnode_device_parse(s5k4h7yx->dev, &props);
	if (ret)
		return ret;

	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 8);
	if (ret)
		return ret;

	/* Serialize controls with sensor device */
	ctrl_hdlr->lock = &s5k4h7yx->mutex;

	/* Initialize exposure and gain */
	lpfr = mode->vblank + mode->height;
	s5k4h7yx->exp_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
					     &s5k4h7yx_ctrl_ops,
					     V4L2_CID_EXPOSURE,
					     S5K4H7YX_EXPOSURE_MIN,
					     lpfr - S5K4H7YX_EXPOSURE_OFFSET,
					     S5K4H7YX_EXPOSURE_STEP,
					     S5K4H7YX_EXPOSURE_DEFAULT);

	s5k4h7yx->again_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
					       &s5k4h7yx_ctrl_ops,
					       V4L2_CID_ANALOGUE_GAIN,
					       S5K4H7YX_ANA_GAIN_MIN,
					       S5K4H7YX_ANA_GAIN_MAX,
					       S5K4H7YX_ANA_GAIN_STEP,
					       S5K4H7YX_ANA_GAIN_DEFAULT);

	v4l2_ctrl_cluster(2, &s5k4h7yx->exp_ctrl);

	s5k4h7yx->vblank_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
						&s5k4h7yx_ctrl_ops,
						V4L2_CID_VBLANK,
						mode->vblank_min,
						mode->vblank_max,
						1, mode->vblank);

	/* Read only controls */
	s5k4h7yx->pclk_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
					      &s5k4h7yx_ctrl_ops,
					      V4L2_CID_PIXEL_RATE,
					      mode->pclk, mode->pclk,
					      1, mode->pclk);

	s5k4h7yx->link_freq_ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr,
							&s5k4h7yx_ctrl_ops,
							V4L2_CID_LINK_FREQ,
							ARRAY_SIZE(link_freq) -
							1,
							mode->link_freq_idx,
							link_freq);
	if (s5k4h7yx->link_freq_ctrl)
		s5k4h7yx->link_freq_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	s5k4h7yx->hblank_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
						&s5k4h7yx_ctrl_ops,
						V4L2_CID_HBLANK,
						S5K4H7YX_REG_MIN,
						S5K4H7YX_REG_MAX,
						1, mode->hblank);
	if (s5k4h7yx->hblank_ctrl)
		s5k4h7yx->hblank_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, &s5k4h7yx_ctrl_ops, &props);

	if (ctrl_hdlr->error) {
		dev_err(s5k4h7yx->dev, "control init failed: %d\n",
			ctrl_hdlr->error);
		v4l2_ctrl_handler_free(ctrl_hdlr);
		return ctrl_hdlr->error;
	}

	s5k4h7yx->sd.ctrl_handler = ctrl_hdlr;

	return 0;
}

/**
 * s5k4h7yx_probe() - I2C client device binding
 * @client: pointer to i2c client device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int s5k4h7yx_probe(struct i2c_client *client)
{
	struct s5k4h7yx *s5k4h7yx;
	int ret;

	s5k4h7yx = devm_kzalloc(&client->dev, sizeof(*s5k4h7yx), GFP_KERNEL);
	if (!s5k4h7yx)
		return -ENOMEM;

	s5k4h7yx->dev = &client->dev;

	/* Initialize subdev */
	v4l2_i2c_subdev_init(&s5k4h7yx->sd, client, &s5k4h7yx_subdev_ops);
	s5k4h7yx->sd.internal_ops = &s5k4h7yx_internal_ops;

	ret = s5k4h7yx_parse_hw_config(s5k4h7yx);
	if (ret) {
		dev_err(s5k4h7yx->dev, "HW configuration is not supported\n");
		return ret;
	}

	mutex_init(&s5k4h7yx->mutex);

	s5k4h7yx->regmap = devm_cci_regmap_init_i2c(client, 16);
	if (IS_ERR(s5k4h7yx->regmap))
		return dev_err_probe(s5k4h7yx->dev, PTR_ERR(s5k4h7yx->regmap),
				     "failed to initialize CCI\n");

	ret = s5k4h7yx_power_on(s5k4h7yx->dev);
	if (ret) {
		dev_err(s5k4h7yx->dev, "failed to power-on the sensor\n");
		goto error_mutex_destroy;
	}

	/* Check module identity */
	ret = s5k4h7yx_detect(s5k4h7yx);
	if (ret) {
		dev_err(s5k4h7yx->dev, "failed to find sensor: %d\n", ret);
		goto error_power_off;
	}

	/* Set default mode to max resolution */
	s5k4h7yx->cur_mode = &supported_mode;
	s5k4h7yx->vblank = s5k4h7yx->cur_mode->vblank;

	ret = s5k4h7yx_init_controls(s5k4h7yx);
	if (ret) {
		dev_err(s5k4h7yx->dev, "failed to init controls: %d\n", ret);
		goto error_power_off;
	}

	/* Initialize subdev */
	s5k4h7yx->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	s5k4h7yx->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	s5k4h7yx->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&s5k4h7yx->sd.entity, 1, &s5k4h7yx->pad);
	if (ret) {
		dev_err(s5k4h7yx->dev, "failed to init entity pads: %d\n", ret);
		goto error_handler_free;
	}

	ret = v4l2_async_register_subdev_sensor(&s5k4h7yx->sd);
	if (ret < 0) {
		dev_err(s5k4h7yx->dev,
			"failed to register async subdev: %d\n", ret);
		goto error_media_entity;
	}

	pm_runtime_set_active(s5k4h7yx->dev);
	pm_runtime_enable(s5k4h7yx->dev);
	pm_runtime_idle(s5k4h7yx->dev);

	return 0;

error_media_entity:
	media_entity_cleanup(&s5k4h7yx->sd.entity);
error_handler_free:
	v4l2_ctrl_handler_free(s5k4h7yx->sd.ctrl_handler);
error_power_off:
	s5k4h7yx_power_off(s5k4h7yx->dev);
error_mutex_destroy:
	mutex_destroy(&s5k4h7yx->mutex);

	return ret;
}

static void s5k4h7yx_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct s5k4h7yx *s5k4h7yx = to_s5k4h7yx(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		s5k4h7yx_power_off(&client->dev);
	pm_runtime_set_suspended(&client->dev);

	mutex_destroy(&s5k4h7yx->mutex);
}

static const struct dev_pm_ops s5k4h7yx_pm_ops = {
	SET_RUNTIME_PM_OPS(s5k4h7yx_power_off, s5k4h7yx_power_on, NULL)
};

static const struct of_device_id s5k4h7yx_of_match[] = {
	{ .compatible = "samsung,s5k4h7yx" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, s5k4h7yx_of_match);

static struct i2c_driver s5k4h7yx_driver = {
	.probe = s5k4h7yx_probe,
	.remove = s5k4h7yx_remove,
	.driver = {
		.name = "s5k4h7yx",
		.pm = &s5k4h7yx_pm_ops,
		.of_match_table = s5k4h7yx_of_match,
	},
};

module_i2c_driver(s5k4h7yx_driver);

MODULE_DESCRIPTION("Samsung S5K4H7YX sensor driver");
MODULE_LICENSE("GPL");

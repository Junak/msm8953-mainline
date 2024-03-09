// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct tianma_n_624_v0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data tianma_n_624_v0_supplies[] = {
	{ .supply = "vsn" },
	{ .supply = "vsp" },
};

static inline
struct tianma_n_624_v0 *to_tianma_n_624_v0(struct drm_panel *panel)
{
	return container_of(panel, struct tianma_n_624_v0, panel);
}

static void tianma_n_624_v0_reset(struct tianma_n_624_v0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int tianma_n_624_v0_on(struct tianma_n_624_v0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x23);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x11, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x12, 0x7f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x15, 0x6e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x16, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_PARTIAL_ROWS, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_PARTIAL_COLUMNS,
				     0xfe);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x32, 0xfc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x33, 0xf8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x34, 0xe5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x35, 0xe0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_ADDRESS_MODE, 0xdc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x37, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x38, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x39, 0xd8);
	mipi_dsi_dcs_set_pixel_format_multi(&dsi_ctx, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x3b, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_3D_CONTROL, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x3f, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_VSYNC_TIMING, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x41, 0xd8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_GET_SCANLINE, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x46, 0xfc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x47, 0xf8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x48, 0xee);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x49, 0xe4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4a, 0xda);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4b, 0xcf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4c, 0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4d, 0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4e, 0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4f, 0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x50, 0xc9);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x00c9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x52, 0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x54, 0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x58, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x59, 0xf9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5a, 0xf2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5b, 0xe4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5c, 0xd4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5d, 0xcd);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS,
				     0xb5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5f, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x60, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x61, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x62, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x63, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x64, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x65, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x66, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x67, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x25);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x05, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0xf0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_READ_PPS_START, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x23);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x07, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x08, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x09, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x0000);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x2c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x68, 0x04, 0x01);

	return dsi_ctx.accum_err;
}

static int tianma_n_624_v0_off(struct tianma_n_624_v0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 35);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int tianma_n_624_v0_prepare(struct drm_panel *panel)
{
	struct tianma_n_624_v0 *ctx = to_tianma_n_624_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(tianma_n_624_v0_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	tianma_n_624_v0_reset(ctx);

	ret = tianma_n_624_v0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(tianma_n_624_v0_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int tianma_n_624_v0_unprepare(struct drm_panel *panel)
{
	struct tianma_n_624_v0 *ctx = to_tianma_n_624_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = tianma_n_624_v0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(tianma_n_624_v0_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode tianma_n_624_v0_mode = {
	.clock = (1080 + 24 + 20 + 44) * (2270 + 14 + 2 + 8) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 24,
	.hsync_end = 1080 + 24 + 20,
	.htotal = 1080 + 24 + 20 + 44,
	.vdisplay = 2270,
	.vsync_start = 2270 + 14,
	.vsync_end = 2270 + 14 + 2,
	.vtotal = 2270 + 14 + 2 + 8,
	.width_mm = 0,
	.height_mm = 0,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int tianma_n_624_v0_get_modes(struct drm_panel *panel,
				     struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &tianma_n_624_v0_mode);
}

static const struct drm_panel_funcs tianma_n_624_v0_panel_funcs = {
	.prepare = tianma_n_624_v0_prepare,
	.unprepare = tianma_n_624_v0_unprepare,
	.get_modes = tianma_n_624_v0_get_modes,
};

static int tianma_n_624_v0_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int tianma_n_624_v0_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness_large(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops tianma_n_624_v0_bl_ops = {
	.update_status = tianma_n_624_v0_bl_update_status,
	.get_brightness = tianma_n_624_v0_bl_get_brightness,
};

static struct backlight_device *
tianma_n_624_v0_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 4095,
		.max_brightness = 4095,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &tianma_n_624_v0_bl_ops, &props);
}

static int tianma_n_624_v0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct tianma_n_624_v0 *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct tianma_n_624_v0, panel,
				   &tianma_n_624_v0_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(tianma_n_624_v0_supplies),
					    tianma_n_624_v0_supplies,
					    &ctx->supplies);
	if (ret < 0)
		return ret;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = tianma_n_624_v0_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void tianma_n_624_v0_remove(struct mipi_dsi_device *dsi)
{
	struct tianma_n_624_v0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id tianma_n_624_v0_of_match[] = {
	{ .compatible = "motorola,river-nt36672a-tianma" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, tianma_n_624_v0_of_match);

static struct mipi_dsi_driver tianma_n_624_v0_driver = {
	.probe = tianma_n_624_v0_probe,
	.remove = tianma_n_624_v0_remove,
	.driver = {
		.name = "panel-tianma-n-624-v0",
		.of_match_table = tianma_n_624_v0_of_match,
	},
};
module_mipi_dsi_driver(tianma_n_624_v0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for mipi_mot_vid_tianma_n_1080p_624");
MODULE_LICENSE("GPL");

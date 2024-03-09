// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

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

struct lg_td4300_5p0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data lg_td4300_5p0_supplies[] = {
	{ .supply = "vsn" },
	{ .supply = "vsp" },
};

static inline struct lg_td4300_5p0 *to_lg_td4300_5p0(struct drm_panel *panel)
{
	return container_of(panel, struct lg_td4300_5p0, panel);
}

static void lg_td4300_5p0_reset(struct lg_td4300_5p0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int lg_td4300_5p0_on(struct lg_td4300_5p0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb3, 0x31, 0x00, 0x06);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb4, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb6,
					 0x33, 0xd3, 0x80, 0xff, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc1,
					 0x80, 0x08, 0x01, 0xef, 0xff, 0x4f,
					 0xa7, 0x8c, 0xf7, 0x98, 0xa5, 0xfe,
					 0x67, 0xcd, 0xb9, 0xc6, 0xfe, 0x57,
					 0xcb, 0x9c, 0x2b, 0x06, 0x29, 0xff,
					 0x7f, 0x00, 0x10, 0x10, 0x10, 0x10,
					 0x10, 0x10, 0x10, 0x10, 0x40, 0x02,
					 0x62, 0x01, 0x06, 0x05, 0x84, 0x00,
					 0x01, 0x00, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc2,
					 0x01, 0xf7, 0x80, 0x0d, 0x69, 0x08,
					 0x0c, 0x10, 0x00, 0x08, 0x60, 0x00,
					 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc3,
					 0x78, 0x77, 0x87, 0x78, 0x70, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x3b, 0x63,
					 0xb6, 0x3b, 0x60, 0x3f, 0xc3, 0xfc,
					 0x3f, 0xc0, 0x01, 0x01, 0x03, 0x28,
					 0x00, 0x01, 0x00, 0x01, 0x11, 0x01,
					 0x13, 0x00, 0x13, 0x00, 0x13, 0x00,
					 0x13, 0x00, 0x13, 0x00, 0x13, 0x00,
					 0x13, 0x00, 0x13, 0x00, 0x13, 0x00,
					 0x2b, 0x00, 0x2b, 0x00, 0x2b, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x40, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc4,
					 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x02, 0x02, 0x25,
					 0x01, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc6,
					 0x62, 0x05, 0x54, 0x05, 0x54, 0x01,
					 0x0e, 0x01, 0x02, 0x01, 0x02, 0x01,
					 0x02, 0x01, 0x02, 0x01, 0x02, 0x08,
					 0x15, 0x06, 0x7f, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc7,
					 0x00, 0x11, 0x1a, 0x27, 0x35, 0x42,
					 0x4b, 0x58, 0x3c, 0x44, 0x50, 0x60,
					 0x69, 0x6f, 0x78, 0x00, 0x11, 0x1a,
					 0x27, 0x35, 0x42, 0x4b, 0x58, 0x3c,
					 0x44, 0x50, 0x60, 0x69, 0x6f, 0x78);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc8,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
					 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc9,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
					 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcb,
					 0xf1, 0x9f, 0x9f, 0xff, 0x08, 0x00,
					 0x10, 0x80, 0x00, 0x00, 0x00, 0x98,
					 0x9f, 0x01, 0x00, 0x00, 0x40);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcc, 0x1e);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcd,
					 0x00, 0x00, 0x22, 0x00, 0x22, 0x00,
					 0x22, 0x00, 0xac, 0xac, 0xa0, 0xa0,
					 0xa0, 0xa0, 0xa0, 0xa0, 0x15, 0x00,
					 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd0,
					 0x33, 0x59, 0xd9, 0x31, 0x01, 0x10,
					 0x10, 0x10, 0x19, 0x19, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd3,
					 0x1b, 0x3b, 0xbb, 0x77, 0x77, 0x77,
					 0xbb, 0xb3, 0x33, 0x00, 0x00, 0x6d,
					 0x6d, 0xd3, 0xd3, 0x33, 0xbb, 0xf2,
					 0xfd, 0xc6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd5,
					 0x03, 0x00, 0x00, 0x00, 0x34, 0x00,
					 0x34);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd6, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_ADDRESS_MODE, 0xc0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf3, 0x02, 0x00, 0xb4);
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 20);

	return dsi_ctx.accum_err;
}

static int lg_td4300_5p0_off(struct lg_td4300_5p0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 32);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int lg_td4300_5p0_prepare(struct drm_panel *panel)
{
	struct lg_td4300_5p0 *ctx = to_lg_td4300_5p0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(lg_td4300_5p0_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	lg_td4300_5p0_reset(ctx);

	ret = lg_td4300_5p0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(lg_td4300_5p0_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int lg_td4300_5p0_unprepare(struct drm_panel *panel)
{
	struct lg_td4300_5p0 *ctx = to_lg_td4300_5p0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = lg_td4300_5p0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(lg_td4300_5p0_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode lg_td4300_5p0_mode = {
	.clock = (1080 + 107 + 2 + 50) * (1920 + 10 + 2 + 6) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 107,
	.hsync_end = 1080 + 107 + 2,
	.htotal = 1080 + 107 + 2 + 50,
	.vdisplay = 1920,
	.vsync_start = 1920 + 10,
	.vsync_end = 1920 + 10 + 2,
	.vtotal = 1920 + 10 + 2 + 6,
	.width_mm = 62,
	.height_mm = 110,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int lg_td4300_5p0_get_modes(struct drm_panel *panel,
				   struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &lg_td4300_5p0_mode);
}

static const struct drm_panel_funcs lg_td4300_5p0_panel_funcs = {
	.prepare = lg_td4300_5p0_prepare,
	.unprepare = lg_td4300_5p0_unprepare,
	.get_modes = lg_td4300_5p0_get_modes,
};

static int lg_td4300_5p0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct lg_td4300_5p0 *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct lg_td4300_5p0, panel,
				   &lg_td4300_5p0_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(lg_td4300_5p0_supplies),
					    lg_td4300_5p0_supplies,
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
			  MIPI_DSI_MODE_VIDEO_HSE |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->panel.prepare_prev_first = true;

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void lg_td4300_5p0_remove(struct mipi_dsi_device *dsi)
{
	struct lg_td4300_5p0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id lg_td4300_5p0_of_match[] = {
	{ .compatible = "huawei,cannes-td4300-lg" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, lg_td4300_5p0_of_match);

static struct mipi_dsi_driver lg_td4300_5p0_driver = {
	.probe = lg_td4300_5p0_probe,
	.remove = lg_td4300_5p0_remove,
	.driver = {
		.name = "panel-lg-td4300-5p0",
		.of_match_table = lg_td4300_5p0_of_match,
	},
};
module_mipi_dsi_driver(lg_td4300_5p0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for LG_TD4300_1080P_VIDEO");
MODULE_LICENSE("GPL");

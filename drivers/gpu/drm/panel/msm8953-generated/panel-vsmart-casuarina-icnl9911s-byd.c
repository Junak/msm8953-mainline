// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct icnl9911s_byd_1600 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data icnl9911s_byd_1600_supplies[] = {
	{ .supply = "vsn" },
	{ .supply = "vsp" },
};

static inline
struct icnl9911s_byd_1600 *to_icnl9911s_byd_1600(struct drm_panel *panel)
{
	return container_of_const(panel, struct icnl9911s_byd_1600, panel);
}

static void icnl9911s_byd_1600_reset(struct icnl9911s_byd_1600 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(30);
}

static int icnl9911s_byd_1600_on(struct icnl9911s_byd_1600 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf0, 0x5a, 0x59);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf1, 0xa5, 0xa6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0,
					 0x82, 0x81, 0x05, 0x04, 0x87, 0x86,
					 0x84, 0x85, 0x66, 0x66, 0x33, 0x33,
					 0x20, 0x01, 0x01, 0x78, 0x01, 0x01,
					 0x0f, 0x05, 0x04, 0x03, 0x02, 0x01,
					 0x02, 0x03, 0x04, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb1,
					 0x11, 0x42, 0x86, 0x00, 0x01, 0x01,
					 0x01, 0x66, 0x01, 0x01, 0x04, 0x08,
					 0x54, 0x00, 0x00, 0x00, 0x44, 0x40,
					 0x02, 0x01, 0x40, 0x02, 0x01, 0x40,
					 0x02, 0x01, 0x40, 0x02, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb2,
					 0x54, 0xc4, 0x82, 0x05, 0x40, 0x02,
					 0x01, 0x40, 0x02, 0x01, 0x05, 0x05,
					 0x54, 0x0c, 0x0c, 0x0d, 0x0b);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb3,
					 0x02, 0x00, 0x00, 0x00, 0x00, 0x26,
					 0x26, 0x91, 0xa2, 0x33, 0x44, 0x00,
					 0x26, 0x00, 0x18, 0x01, 0x02, 0x08,
					 0x20, 0x30, 0x08, 0x09, 0x44, 0x20,
					 0x40, 0x20, 0x40, 0x08, 0x09, 0x22,
					 0x33);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb4,
					 0x0a, 0x02, 0xdc, 0x1d, 0x00, 0x02,
					 0x02, 0x02, 0x02, 0x12, 0x10, 0x02,
					 0x02, 0x0e, 0x0c, 0x04, 0x03, 0x03,
					 0x03, 0x03, 0x03, 0x03, 0xff, 0xff,
					 0xfc, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb5,
					 0x0b, 0x02, 0xdc, 0x1d, 0x00, 0x02,
					 0x02, 0x02, 0x02, 0x13, 0x11, 0x02,
					 0x02, 0x0f, 0x0d, 0x05, 0x03, 0x03,
					 0x03, 0x03, 0x03, 0x03, 0xff, 0xff,
					 0xfc, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb8,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbb,
					 0x01, 0x05, 0x09, 0x11, 0x0d, 0x19,
					 0x1d, 0x15, 0x25, 0x69, 0x00, 0x21,
					 0x25);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbc,
					 0x00, 0x00, 0x00, 0x00, 0x02, 0x20,
					 0xff, 0x00, 0x03, 0x33, 0x01, 0x73,
					 0x44, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbd,
					 0x53, 0x12, 0x4f, 0xcf, 0x72, 0xa7,
					 0x08, 0x44, 0xae, 0x15);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbe,
					 0x65, 0x65, 0x50, 0x46, 0x0c, 0x66,
					 0x43, 0x06, 0x0e, 0x0e);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbf,
					 0x07, 0x25, 0x07, 0x25, 0x7f, 0x00,
					 0x11, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc0,
					 0x10, 0xff, 0xff, 0xff, 0xff, 0xff,
					 0x00, 0xff, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc1,
					 0xc0, 0x0c, 0x20, 0x7c, 0x04, 0x0c,
					 0x10, 0x04, 0x2a, 0x40, 0x36, 0x00,
					 0x07, 0xcf, 0xff, 0xff, 0xc0, 0x00,
					 0xc0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc2, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc3,
					 0x06, 0x00, 0xff, 0x00, 0xff, 0x00,
					 0x00, 0x81, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc5,
					 0x03, 0x1c, 0xc0, 0xc0, 0x40, 0x10,
					 0x42, 0x44, 0x08, 0x09, 0x14);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc6,
					 0x87, 0x96, 0x2a, 0x29, 0x29, 0x31,
					 0x7f, 0x34, 0x08, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc7,
					 0xf7, 0xb7, 0x8e, 0x70, 0x42, 0x21,
					 0xf1, 0x46, 0x15, 0xef, 0xc9, 0x9d,
					 0xf7, 0xcb, 0xad, 0x85, 0x6b, 0x46,
					 0x1a, 0x7e, 0xc0, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc8,
					 0xf7, 0xb7, 0x8e, 0x70, 0x42, 0x21,
					 0xf1, 0x46, 0x15, 0xef, 0xc9, 0x9d,
					 0xf7, 0xcb, 0xad, 0x85, 0x6b, 0x46,
					 0x1a, 0x7e, 0xc0, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcb, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd0,
					 0x80, 0x0d, 0xff, 0x0f, 0x63);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd2, 0x42);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf1, 0x5a, 0x59);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x35, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x11, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x29, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x26, 0x01);

	return dsi_ctx.accum_err;
}

static int icnl9911s_byd_1600_off(struct icnl9911s_byd_1600 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x26, 0x08);
	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 50);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int icnl9911s_byd_1600_prepare(struct drm_panel *panel)
{
	struct icnl9911s_byd_1600 *ctx = to_icnl9911s_byd_1600(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(icnl9911s_byd_1600_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	icnl9911s_byd_1600_reset(ctx);

	ret = icnl9911s_byd_1600_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(icnl9911s_byd_1600_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int icnl9911s_byd_1600_unprepare(struct drm_panel *panel)
{
	struct icnl9911s_byd_1600 *ctx = to_icnl9911s_byd_1600(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = icnl9911s_byd_1600_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(icnl9911s_byd_1600_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode icnl9911s_byd_1600_mode = {
	.clock = (720 + 300 + 10 + 60) * (1600 + 124 + 4 + 12) * 60 / 1000,
	.hdisplay = 720,
	.hsync_start = 720 + 300,
	.hsync_end = 720 + 300 + 10,
	.htotal = 720 + 300 + 10 + 60,
	.vdisplay = 1600,
	.vsync_start = 1600 + 124,
	.vsync_end = 1600 + 124 + 4,
	.vtotal = 1600 + 124 + 4 + 12,
	.width_mm = 0,
	.height_mm = 0,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int icnl9911s_byd_1600_get_modes(struct drm_panel *panel,
					struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &icnl9911s_byd_1600_mode);
}

static const struct drm_panel_funcs icnl9911s_byd_1600_panel_funcs = {
	.prepare = icnl9911s_byd_1600_prepare,
	.unprepare = icnl9911s_byd_1600_unprepare,
	.get_modes = icnl9911s_byd_1600_get_modes,
};

static int icnl9911s_byd_1600_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct icnl9911s_byd_1600 *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct icnl9911s_byd_1600, panel,
				   &icnl9911s_byd_1600_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(icnl9911s_byd_1600_supplies),
					    icnl9911s_byd_1600_supplies,
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
			  MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_VIDEO_NO_HFP;

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

static void icnl9911s_byd_1600_remove(struct mipi_dsi_device *dsi)
{
	struct icnl9911s_byd_1600 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id icnl9911s_byd_1600_of_match[] = {
	{ .compatible = "vsmart,casuarina-icnl9911s-byd" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, icnl9911s_byd_1600_of_match);

static struct mipi_dsi_driver icnl9911s_byd_1600_driver = {
	.probe = icnl9911s_byd_1600_probe,
	.remove = icnl9911s_byd_1600_remove,
	.driver = {
		.name = "panel-icnl9911s-byd-1600",
		.of_match_table = icnl9911s_byd_1600_of_match,
	},
};
module_mipi_dsi_driver(icnl9911s_byd_1600_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for icnl9911s byd 1600 videodsi panel");
MODULE_LICENSE("GPL");

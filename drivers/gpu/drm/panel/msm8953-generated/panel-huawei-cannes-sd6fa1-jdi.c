// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025 FIXME
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

struct jdi_sd6fa1_5p0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data jdi_sd6fa1_5p0_supplies[] = {
	{ .supply = "vsn" },
	{ .supply = "vsp" },
};

static inline struct jdi_sd6fa1_5p0 *to_jdi_sd6fa1_5p0(struct drm_panel *panel)
{
	return container_of(panel, struct jdi_sd6fa1_5p0, panel);
}

static void jdi_sd6fa1_5p0_reset(struct jdi_sd6fa1_5p0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int jdi_sd6fa1_5p0_on(struct jdi_sd6fa1_5p0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf1, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe0, 0x01, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe2, 0xf5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf1, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_set_tear_on_multi(&dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x11, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x29, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 20);

	return dsi_ctx.accum_err;
}

static int jdi_sd6fa1_5p0_off(struct jdi_sd6fa1_5p0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x28, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 32);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x10, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int jdi_sd6fa1_5p0_prepare(struct drm_panel *panel)
{
	struct jdi_sd6fa1_5p0 *ctx = to_jdi_sd6fa1_5p0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(jdi_sd6fa1_5p0_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	jdi_sd6fa1_5p0_reset(ctx);

	ret = jdi_sd6fa1_5p0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(jdi_sd6fa1_5p0_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int jdi_sd6fa1_5p0_unprepare(struct drm_panel *panel)
{
	struct jdi_sd6fa1_5p0 *ctx = to_jdi_sd6fa1_5p0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = jdi_sd6fa1_5p0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(jdi_sd6fa1_5p0_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode jdi_sd6fa1_5p0_mode = {
	.clock = (1080 + 116 + 8 + 48) * (1920 + 4 + 4 + 3) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 116,
	.hsync_end = 1080 + 116 + 8,
	.htotal = 1080 + 116 + 8 + 48,
	.vdisplay = 1920,
	.vsync_start = 1920 + 4,
	.vsync_end = 1920 + 4 + 4,
	.vtotal = 1920 + 4 + 4 + 3,
	.width_mm = 68,
	.height_mm = 121,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int jdi_sd6fa1_5p0_get_modes(struct drm_panel *panel,
				    struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &jdi_sd6fa1_5p0_mode);
}

static const struct drm_panel_funcs jdi_sd6fa1_5p0_panel_funcs = {
	.prepare = jdi_sd6fa1_5p0_prepare,
	.unprepare = jdi_sd6fa1_5p0_unprepare,
	.get_modes = jdi_sd6fa1_5p0_get_modes,
};

static int jdi_sd6fa1_5p0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct jdi_sd6fa1_5p0 *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct jdi_sd6fa1_5p0, panel,
				   &jdi_sd6fa1_5p0_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(jdi_sd6fa1_5p0_supplies),
					    jdi_sd6fa1_5p0_supplies,
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

static void jdi_sd6fa1_5p0_remove(struct mipi_dsi_device *dsi)
{
	struct jdi_sd6fa1_5p0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id jdi_sd6fa1_5p0_of_match[] = {
	{ .compatible = "huawei,cannes-sd6fa1-jdi" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, jdi_sd6fa1_5p0_of_match);

static struct mipi_dsi_driver jdi_sd6fa1_5p0_driver = {
	.probe = jdi_sd6fa1_5p0_probe,
	.remove = jdi_sd6fa1_5p0_remove,
	.driver = {
		.name = "panel-jdi-sd6fa1-5p0",
		.of_match_table = jdi_sd6fa1_5p0_of_match,
	},
};
module_mipi_dsi_driver(jdi_sd6fa1_5p0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for JDI_SD6FA1_5P0_1080P_VIDEO");
MODULE_LICENSE("GPL");

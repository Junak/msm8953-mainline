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

struct tianma_h_624_v0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data tianma_h_624_v0_supplies[] = {
	{ .supply = "vsn" },
	{ .supply = "vsp" },
};

static inline
struct tianma_h_624_v0 *to_tianma_h_624_v0(struct drm_panel *panel)
{
	return container_of(panel, struct tianma_h_624_v0, panel);
}

static void tianma_h_624_v0_reset(struct tianma_h_624_v0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(2000, 3000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(2000, 3000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(50);
}

static int tianma_h_624_v0_on(struct tianma_h_624_v0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb9, 0x83, 0x11, 0x2a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcf, 0x00, 0x14, 0x00, 0xc0);
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 90);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 20);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x0000);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x2c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x01);
	mipi_dsi_usleep_range(&dsi_ctx, 5000, 6000);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe4,
				     0x2d, 0x01, 0x2c, 0x00, 0x08, 0x00, 0x10,
				     0x08, 0x04, 0x04, 0x8d, 0x8d, 0x8d, 0x99,
				     0x99, 0xc2, 0xc2, 0xff, 0xff, 0xff, 0xff,
				     0xef);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x04, 0x08, 0xb9, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x73, 0x23);

	return dsi_ctx.accum_err;
}

static int tianma_h_624_v0_off(struct tianma_h_624_v0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 35);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int tianma_h_624_v0_prepare(struct drm_panel *panel)
{
	struct tianma_h_624_v0 *ctx = to_tianma_h_624_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(tianma_h_624_v0_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	tianma_h_624_v0_reset(ctx);

	ret = tianma_h_624_v0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(tianma_h_624_v0_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int tianma_h_624_v0_unprepare(struct drm_panel *panel)
{
	struct tianma_h_624_v0 *ctx = to_tianma_h_624_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = tianma_h_624_v0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(tianma_h_624_v0_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode tianma_h_624_v0_mode = {
	.clock = (1080 + 25 + 24 + 24) * (2270 + 42 + 5 + 7) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 25,
	.hsync_end = 1080 + 25 + 24,
	.htotal = 1080 + 25 + 24 + 24,
	.vdisplay = 2270,
	.vsync_start = 2270 + 42,
	.vsync_end = 2270 + 42 + 5,
	.vtotal = 2270 + 42 + 5 + 7,
	.width_mm = 0,
	.height_mm = 0,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int tianma_h_624_v0_get_modes(struct drm_panel *panel,
				     struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &tianma_h_624_v0_mode);
}

static const struct drm_panel_funcs tianma_h_624_v0_panel_funcs = {
	.prepare = tianma_h_624_v0_prepare,
	.unprepare = tianma_h_624_v0_unprepare,
	.get_modes = tianma_h_624_v0_get_modes,
};

static int tianma_h_624_v0_bl_update_status(struct backlight_device *bl)
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
static int tianma_h_624_v0_bl_get_brightness(struct backlight_device *bl)
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

static const struct backlight_ops tianma_h_624_v0_bl_ops = {
	.update_status = tianma_h_624_v0_bl_update_status,
	.get_brightness = tianma_h_624_v0_bl_get_brightness,
};

static struct backlight_device *
tianma_h_624_v0_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 4095,
		.max_brightness = 4095,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &tianma_h_624_v0_bl_ops, &props);
}

static int tianma_h_624_v0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct tianma_h_624_v0 *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct tianma_h_624_v0, panel,
				   &tianma_h_624_v0_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(tianma_h_624_v0_supplies),
					    tianma_h_624_v0_supplies,
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
			  MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = tianma_h_624_v0_create_backlight(dsi);
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

static void tianma_h_624_v0_remove(struct mipi_dsi_device *dsi)
{
	struct tianma_h_624_v0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id tianma_h_624_v0_of_match[] = {
	{ .compatible = "motorola,river-hx83112b-tianma" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, tianma_h_624_v0_of_match);

static struct mipi_dsi_driver tianma_h_624_v0_driver = {
	.probe = tianma_h_624_v0_probe,
	.remove = tianma_h_624_v0_remove,
	.driver = {
		.name = "panel-tianma-h-624-v0",
		.of_match_table = tianma_h_624_v0_of_match,
	},
};
module_mipi_dsi_driver(tianma_h_624_v0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for mipi_mot_vid_tianma_h_1080p_624");
MODULE_LICENSE("GPL");

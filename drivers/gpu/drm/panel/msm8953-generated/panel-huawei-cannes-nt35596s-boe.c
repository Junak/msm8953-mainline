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

struct boe_nt35596s_5p0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data boe_nt35596s_5p0_supplies[] = {
	{ .supply = "vsn" },
	{ .supply = "vsp" },
};

static inline
struct boe_nt35596s_5p0 *to_boe_nt35596s_5p0(struct drm_panel *panel)
{
	return container_of(panel, struct boe_nt35596s_5p0, panel);
}

static void boe_nt35596s_5p0_reset(struct boe_nt35596s_5p0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int boe_nt35596s_5p0_on(struct boe_nt35596s_5p0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0xe0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4f, 0x01);
	mipi_dsi_usleep_range(&dsi_ctx, 1000, 2000);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4f, 0x00);
	mipi_dsi_usleep_range(&dsi_ctx, 1000, 2000);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x2e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x03, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x67, 0x0c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x75, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x76, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x77, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x78, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x79, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7a, 0x36);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7b, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7c, 0x50);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7d, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7e, 0x66);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7f, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x80, 0x7a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x81, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x82, 0x8d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x83, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x84, 0x9d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x85, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x86, 0xac);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x87, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x88, 0xdf);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x89, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8a, 0x05);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8b, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8c, 0x3f);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8d, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8e, 0x6a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8f, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x90, 0xab);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x91, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x92, 0xde);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x93, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x94, 0xe0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x95, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x96, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x97, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x98, 0x4d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x99, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9a, 0x76);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9b, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9c, 0xb1);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9d, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9e, 0xdd);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9f, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa0, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa2, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa3, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa4, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa5, 0x31);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa6, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa7, 0x45);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa9, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xaa, 0x5c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xab, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xac, 0x77);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xad, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xae, 0x9a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xaf, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0xc6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb1, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb2, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb3, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb4, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb5, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb6, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb7, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb8, 0x36);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb9, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xba, 0x50);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbb, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbc, 0x66);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbd, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbe, 0x7a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbf, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc0, 0x8d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc1, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc2, 0x9d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc3, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc4, 0xac);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc5, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc6, 0xdf);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc7, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc8, 0x05);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc9, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xca, 0x3f);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcc, 0x6a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcd, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xce, 0xab);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcf, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd0, 0xde);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd1, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd2, 0xe0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd3, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd4, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd5, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd6, 0x4d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd7, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd8, 0x76);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd9, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xda, 0xb1);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdb, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdc, 0xdd);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdd, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xde, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdf, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe0, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe1, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe2, 0x31);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe3, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe4, 0x45);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe5, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe6, 0x5c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe7, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe8, 0x77);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe9, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xea, 0x9a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xeb, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xec, 0xc6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xed, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xee, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xef, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf0, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf1, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf2, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf3, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf4, 0x36);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf5, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf6, 0x50);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf7, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf8, 0x66);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xf9, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfa, 0x7a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x21);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x01, 0x8d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x02, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x03, 0x9d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x04, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x05, 0xac);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x06, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x07, 0xdf);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x08, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x09, 0x05);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0a, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0b, 0x3f);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0c, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0d, 0x6a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0e, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0f, 0xab);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x10, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x11, 0xde);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x12, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x13, 0xe0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x14, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x15, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x16, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x17, 0x4d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x18, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x19, 0x76);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1a, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1b, 0xb1);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1c, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1d, 0xdd);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1e, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1f, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x20, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x21, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x22, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x23, 0x31);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x24, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x25, 0x45);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x26, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x27, 0x5c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x28, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x29, 0x77);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x2a, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x2b, 0x9a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x2d, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x2f, 0xc6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x30, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x31, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x32, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x33, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x34, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x35, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x36, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x37, 0x36);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x38, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x39, 0x50);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3a, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3b, 0x66);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3d, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3f, 0x7a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x40, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x41, 0x8d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x42, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x43, 0x9d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x44, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x45, 0xac);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x46, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x47, 0xdf);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x48, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x49, 0x05);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4a, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4b, 0x3f);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4c, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4d, 0x6a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4e, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4f, 0xab);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x50, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x51, 0xde);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x52, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x53, 0xe0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x54, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x55, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x56, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x58, 0x4d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x59, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x5a, 0x76);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x5b, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x5c, 0xb1);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x5d, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x5e, 0xdd);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x5f, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x60, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x61, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x62, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x63, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x64, 0x31);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x65, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x66, 0x45);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x67, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x68, 0x5c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x69, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x6a, 0x77);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x6b, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x6c, 0x9a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x6d, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x6e, 0xc6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x6f, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x70, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x71, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x72, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x73, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x74, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x75, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x76, 0x36);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x77, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x78, 0x50);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x79, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7a, 0x66);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7b, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7c, 0x7a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7d, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7e, 0x8d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x7f, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x80, 0x9d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x81, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x82, 0xac);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x83, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x84, 0xdf);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x85, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x86, 0x05);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x87, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x88, 0x3f);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x89, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8a, 0x6a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8b, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8c, 0xab);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8d, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8e, 0xde);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x8f, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x90, 0xe0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x91, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x92, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x93, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x94, 0x4d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x95, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x96, 0x76);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x97, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x98, 0xb1);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x99, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9a, 0xdd);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9b, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9c, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9d, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9e, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9f, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa0, 0x31);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa2, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa3, 0x45);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa4, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa5, 0x5c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa6, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa7, 0x77);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa9, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xaa, 0x9a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xab, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xac, 0xc6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xad, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xae, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xaf, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb1, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb2, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb3, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb4, 0x36);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb5, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb6, 0x50);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb7, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb8, 0x66);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb9, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xba, 0x7a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbb, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbc, 0x8d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbd, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbe, 0x9d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbf, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc0, 0xac);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc1, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc2, 0xdf);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc3, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc4, 0x05);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc5, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc6, 0x3f);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc7, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc8, 0x6a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc9, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xca, 0xab);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcc, 0xde);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcd, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xce, 0xe0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcf, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd0, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd1, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd2, 0x4d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd3, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd4, 0x76);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd5, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd6, 0xb1);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd7, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd8, 0xdd);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd9, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xda, 0x16);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdb, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdc, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdd, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xde, 0x31);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xdf, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe0, 0x45);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe1, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe2, 0x5c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe3, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe4, 0x77);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe5, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe6, 0x9a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe7, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe8, 0xc6);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xe9, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xea, 0xff);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x55, 0x80);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x22);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x01, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x02, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x03, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x04, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x05, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x06, 0x08);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x07, 0x0c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x08, 0x14);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x09, 0x24);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0a, 0x32);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0b, 0x41);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0c, 0x44);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0d, 0x46);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0e, 0x34);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x0f, 0x23);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x10, 0x0c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x11, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x12, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x13, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1a, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1b, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1c, 0x08);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1d, 0x14);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1e, 0x1e);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1f, 0x2a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x20, 0x34);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x21, 0x1a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x22, 0x08);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x23, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x24, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x25, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x26, 0x1a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x27, 0x28);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x28, 0x2a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x29, 0x1a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x2a, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x2b, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x2f, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x30, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x31, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x32, 0x82);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x33, 0x84);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x34, 0x06);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x35, 0x09);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x36, 0x0c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x37, 0x0e);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x38, 0x0e);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x39, 0x0c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3a, 0x08);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3b, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3f, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x40, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x41, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x42, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x43, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x44, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x45, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x46, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x47, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x48, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x49, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4a, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4b, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4c, 0x84);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4d, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4e, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x4f, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x50, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x51, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x52, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x53, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x54, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x55, 0x99);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x56, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x58, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x68, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x84, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x85, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x86, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x87, 0x80);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x88, 0x07);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa2, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa9, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xaa, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xab, 0x0d);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xac, 0x07);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xad, 0x64);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xaf, 0x22);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb1, 0x12);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb2, 0x2f);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb3, 0x29);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb4, 0xf0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb6, 0x33);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb7, 0x03);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb8, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb9, 0x55);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xba, 0x4a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbb, 0xa5);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbd, 0x22);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbe, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xbf, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc0, 0x75);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc1, 0x6a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc2, 0xa5);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc4, 0x22);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc5, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc6, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc7, 0x95);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc8, 0x8a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc9, 0xa5);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcb, 0x22);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcc, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcd, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xce, 0xb5);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xcf, 0xaa);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd0, 0xa5);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd2, 0x22);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd3, 0x02);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9f, 0xf7);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa0, 0x33);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x89, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9d, 0xfc);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xa0, 0x32);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x89, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x11, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x29, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 20);

	return dsi_ctx.accum_err;
}

static int boe_nt35596s_5p0_off(struct boe_nt35596s_5p0 *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x10, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x28, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 32);

	return dsi_ctx.accum_err;
}

static int boe_nt35596s_5p0_prepare(struct drm_panel *panel)
{
	struct boe_nt35596s_5p0 *ctx = to_boe_nt35596s_5p0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(boe_nt35596s_5p0_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	boe_nt35596s_5p0_reset(ctx);

	ret = boe_nt35596s_5p0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(boe_nt35596s_5p0_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int boe_nt35596s_5p0_unprepare(struct drm_panel *panel)
{
	struct boe_nt35596s_5p0 *ctx = to_boe_nt35596s_5p0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = boe_nt35596s_5p0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(boe_nt35596s_5p0_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode boe_nt35596s_5p0_mode = {
	.clock = (1080 + 114 + 8 + 50) * (1920 + 8 + 4 + 6) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 114,
	.hsync_end = 1080 + 114 + 8,
	.htotal = 1080 + 114 + 8 + 50,
	.vdisplay = 1920,
	.vsync_start = 1920 + 8,
	.vsync_end = 1920 + 8 + 4,
	.vtotal = 1920 + 8 + 4 + 6,
	.width_mm = 62,
	.height_mm = 110,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int boe_nt35596s_5p0_get_modes(struct drm_panel *panel,
				      struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &boe_nt35596s_5p0_mode);
}

static const struct drm_panel_funcs boe_nt35596s_5p0_panel_funcs = {
	.prepare = boe_nt35596s_5p0_prepare,
	.unprepare = boe_nt35596s_5p0_unprepare,
	.get_modes = boe_nt35596s_5p0_get_modes,
};

static int boe_nt35596s_5p0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct boe_nt35596s_5p0 *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct boe_nt35596s_5p0, panel,
				   &boe_nt35596s_5p0_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(boe_nt35596s_5p0_supplies),
					    boe_nt35596s_5p0_supplies,
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
			  MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_NO_EOT_PACKET |
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

static void boe_nt35596s_5p0_remove(struct mipi_dsi_device *dsi)
{
	struct boe_nt35596s_5p0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id boe_nt35596s_5p0_of_match[] = {
	{ .compatible = "huawei,cannes-nt35596s-boe" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, boe_nt35596s_5p0_of_match);

static struct mipi_dsi_driver boe_nt35596s_5p0_driver = {
	.probe = boe_nt35596s_5p0_probe,
	.remove = boe_nt35596s_5p0_remove,
	.driver = {
		.name = "panel-boe-nt35596s-5p0",
		.of_match_table = boe_nt35596s_5p0_of_match,
	},
};
module_mipi_dsi_driver(boe_nt35596s_5p0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for BOE_NT35596S_5P0_1080P_VIDEO");
MODULE_LICENSE("GPL");

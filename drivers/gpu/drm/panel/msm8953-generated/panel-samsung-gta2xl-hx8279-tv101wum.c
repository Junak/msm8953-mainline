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

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct hx8279_tv101wum_wuxga {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data hx8279_tv101wum_wuxga_supplies[] = {
	{ .supply = "vsn" },
	{ .supply = "vsp" },
};

static inline
struct hx8279_tv101wum_wuxga *to_hx8279_tv101wum_wuxga(struct drm_panel *panel)
{
	return container_of(panel, struct hx8279_tv101wum_wuxga, panel);
}

static void hx8279_tv101wum_wuxga_reset(struct hx8279_tv101wum_wuxga *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(7000, 8000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(7000, 8000);
}

static int hx8279_tv101wum_wuxga_on(struct hx8279_tv101wum_wuxga *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe0, 0x26);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe1, 0x26);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdc, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdd, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0x26);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0x26);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd2, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd3, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe6, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe7, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0x09);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x09);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd8, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd9, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd6, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd7, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd4, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd5, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xde, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdf, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe7, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x2a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xde, 0x2a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x43);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe4, 0xc0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe5, 0x0d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x87);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbf, 0x1f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0x2b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x39);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0x36);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x34);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x35);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x36);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0x39);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0x2d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce, 0x2e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcf, 0x2f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd0, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd2, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd3, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd4, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd5, 0x2b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd6, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd7, 0x39);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd8, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd9, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xda, 0x36);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdb, 0x34);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdc, 0x35);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdd, 0x36);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xde, 0x39);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdf, 0x2d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe0, 0x2e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe1, 0x2f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe2, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x52);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0xa5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0xa5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x33);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd5, 0x32);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb1, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb4, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x29);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0x79);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb9, 0xbf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbb, 0x88);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x14);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbd, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbe, 0x97);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbf, 0x11);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x4b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x82);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x9b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0xb6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0xc3);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0xd0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6, 0xdb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0xe1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0xe4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x16);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb1, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb4, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x29);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0x7a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb9, 0xc1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbb, 0x8b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x17);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbd, 0x1b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbe, 0x99);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbf, 0x13);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x4c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x84);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x9d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0xb7);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0xc4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0xd0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6, 0xdb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0xe1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0xe4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x16);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x09);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb1, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x09);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb4, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x2b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x3a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0x5d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb9, 0xca);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x13);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbb, 0x9d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x30);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbd, 0x34);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbe, 0xbb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbf, 0x30);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x6a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0xa1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0xbc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0xd4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0xe0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0xeb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6, 0xf6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0xfa);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0xfc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x16);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb1, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb4, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x29);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0x79);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb9, 0xbf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbb, 0x88);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x14);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbd, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbe, 0x97);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbf, 0x11);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x4b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x82);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x9b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0xb6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0xc3);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0xd0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6, 0xdb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0xe1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0xe4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x16);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb1, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb4, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x29);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0x7a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb9, 0xc1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbb, 0x8b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x17);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbd, 0x1b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbe, 0x99);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbf, 0x13);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x4c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x84);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x9d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0xb7);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0xc4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0xd0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6, 0xdb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0xe1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0xe4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x16);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb1, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x09);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb4, 0x1a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x2b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb6, 0x3a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0x5d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb9, 0xca);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xba, 0x13);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbb, 0x9d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x30);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbd, 0x34);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbe, 0xbb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbf, 0x30);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x6a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0xa1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0xbc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0xd4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc4, 0xe0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0xeb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6, 0xf6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc7, 0xfa);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc8, 0xfc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x16);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcc, 0xaf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcd, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce, 0xff);
	mipi_dsi_msleep(&dsi_ctx, 100);

	return dsi_ctx.accum_err;
}

static int hx8279_tv101wum_wuxga_off(struct hx8279_tv101wum_wuxga *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 100);

	return dsi_ctx.accum_err;
}

static int hx8279_tv101wum_wuxga_prepare(struct drm_panel *panel)
{
	struct hx8279_tv101wum_wuxga *ctx = to_hx8279_tv101wum_wuxga(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(hx8279_tv101wum_wuxga_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	hx8279_tv101wum_wuxga_reset(ctx);

	ret = hx8279_tv101wum_wuxga_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(hx8279_tv101wum_wuxga_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int hx8279_tv101wum_wuxga_unprepare(struct drm_panel *panel)
{
	struct hx8279_tv101wum_wuxga *ctx = to_hx8279_tv101wum_wuxga(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = hx8279_tv101wum_wuxga_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(hx8279_tv101wum_wuxga_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode hx8279_tv101wum_wuxga_mode = {
	.clock = (1200 + 20 + 24 + 46) * (1920 + 30 + 1 + 13) * 60 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1200 + 20,
	.hsync_end = 1200 + 20 + 24,
	.htotal = 1200 + 20 + 24 + 46,
	.vdisplay = 1920,
	.vsync_start = 1920 + 30,
	.vsync_end = 1920 + 30 + 1,
	.vtotal = 1920 + 30 + 1 + 13,
	.width_mm = 141,
	.height_mm = 226,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int hx8279_tv101wum_wuxga_get_modes(struct drm_panel *panel,
					   struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &hx8279_tv101wum_wuxga_mode);
}

static const struct drm_panel_funcs hx8279_tv101wum_wuxga_panel_funcs = {
	.prepare = hx8279_tv101wum_wuxga_prepare,
	.unprepare = hx8279_tv101wum_wuxga_unprepare,
	.get_modes = hx8279_tv101wum_wuxga_get_modes,
};

static int hx8279_tv101wum_wuxga_bl_update_status(struct backlight_device *bl)
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
static int hx8279_tv101wum_wuxga_bl_get_brightness(struct backlight_device *bl)
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

static const struct backlight_ops hx8279_tv101wum_wuxga_bl_ops = {
	.update_status = hx8279_tv101wum_wuxga_bl_update_status,
	.get_brightness = hx8279_tv101wum_wuxga_bl_get_brightness,
};

static struct backlight_device *
hx8279_tv101wum_wuxga_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 256,
		.max_brightness = 256,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &hx8279_tv101wum_wuxga_bl_ops, &props);
}

static int hx8279_tv101wum_wuxga_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hx8279_tv101wum_wuxga *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct hx8279_tv101wum_wuxga, panel,
				   &hx8279_tv101wum_wuxga_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(hx8279_tv101wum_wuxga_supplies),
					    hx8279_tv101wum_wuxga_supplies,
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

	ctx->panel.backlight = hx8279_tv101wum_wuxga_create_backlight(dsi);
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

static void hx8279_tv101wum_wuxga_remove(struct mipi_dsi_device *dsi)
{
	struct hx8279_tv101wum_wuxga *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id hx8279_tv101wum_wuxga_of_match[] = {
	{ .compatible = "samsung,gta2xl-hx8279-tv101wum" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hx8279_tv101wum_wuxga_of_match);

static struct mipi_dsi_driver hx8279_tv101wum_wuxga_driver = {
	.probe = hx8279_tv101wum_wuxga_probe,
	.remove = hx8279_tv101wum_wuxga_remove,
	.driver = {
		.name = "panel-hx8279-tv101wum-wuxga",
		.of_match_table = hx8279_tv101wum_wuxga_of_match,
	},
};
module_mipi_dsi_driver(hx8279_tv101wum_wuxga_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for ss_dsi_panel_HX8279_TV101WUM_WUXGA");
MODULE_LICENSE("GPL");

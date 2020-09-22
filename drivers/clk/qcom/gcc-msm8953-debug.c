/*
 * Copyright (c) 2015-2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/spinlock.h>

static DEFINE_SPINLOCK(local_clock_reg_lock);
static void __iomem *gcc_base;
static struct dentry *rootdir;

struct debug_mux_item {
	const char *name;
	u16 value;
	u16 apcs_mult :5;
	u16 apcs_mux_val :3;
};

static struct debug_mux_item msm8953_mux_parents[] = {
	{ "apcs_c0_clk", 0x16a, 16, 0x0 },
	{ "apcs_c1_clk", 0x16a, 16, 0x1 },
	{ "apcs_cci_clk", 0x16a, 4, 0x2 },
	{ "snoc_clk", 0x000 },
	{ "sysmmnoc_clk", 0x001 },
	{ "pcnoc_clk", 0x008 },
	{ "bimc_clk", 0x15a },
	{ "ipa_clk", 0x1b0 },
	{ "gcc_dcc_clk", 0x00d },
	{ "gcc_pcnoc_usb3_axi_clk", 0x00e },
	{ "gcc_gp1_clk", 0x010 },
	{ "gcc_gp2_clk", 0x011 },
	{ "gcc_gp3_clk", 0x012 },
	{ "gcc_apc0_droop_detector_gpll0_clk", 0x01c },
	{ "gcc_camss_csi2phytimer_clk", 0x01d },
	{ "gcc_apc1_droop_detector_gpll0_clk", 0x01f },
	{ "gcc_bimc_gfx_clk", 0x02d },
	{ "gcc_mss_cfg_ahb_clk", 0x030 },
	{ "gcc_mss_q6_bimc_axi_clk", 0x031 },
	{ "gcc_qdss_dap_clk", 0x049 },
	{ "gcc_apss_tcu_async_clk", 0x050 },
	{ "gcc_mdp_tbu_clk", 0x051 },
	{ "gcc_venus_tbu_clk", 0x054 },
	{ "gcc_vfe_tbu_clk", 0x05a },
	{ "gcc_smmu_cfg_clk", 0x05b },
	{ "gcc_jpeg_tbu_clk", 0x05c },
	{ "gcc_usb30_master_clk", 0x060 },
	{ "gcc_usb30_sleep_clk", 0x061 },
	{ "gcc_usb30_mock_utmi_clk", 0x062 },
	{ "gcc_usb_phy_cfg_ahb_clk", 0x063 },
	{ "gcc_usb3_pipe_clk", 0x066 },
	{ "gcc_usb3_aux_clk", 0x067 },
	{ "gcc_sdcc1_apps_clk", 0x068 },
	{ "gcc_sdcc1_ahb_clk", 0x069 },
	{ "gcc_sdcc1_ice_core_clk", 0x06a },
	{ "gcc_sdcc2_apps_clk", 0x070 },
	{ "gcc_sdcc2_ahb_clk", 0x071 },
	{ "gcc_blsp1_ahb_clk", 0x088 },
	{ "gcc_blsp1_qup1_spi_apps_clk", 0x08a },
	{ "gcc_blsp1_qup1_i2c_apps_clk", 0x08b },
	{ "gcc_blsp1_uart1_apps_clk", 0x08c },
	{ "gcc_blsp1_qup2_spi_apps_clk", 0x08e },
	{ "gcc_blsp1_qup2_i2c_apps_clk", 0x090 },
	{ "gcc_blsp1_uart2_apps_clk", 0x091 },
	{ "gcc_blsp1_qup3_spi_apps_clk", 0x093 },
	{ "gcc_blsp1_qup3_i2c_apps_clk", 0x094 },
	{ "gcc_blsp1_qup4_spi_apps_clk", 0x095 },
	{ "gcc_blsp1_qup4_i2c_apps_clk", 0x096 },
	{ "gcc_blsp2_ahb_clk", 0x098 },
	{ "gcc_blsp2_qup1_spi_apps_clk", 0x09a },
	{ "gcc_blsp2_qup1_i2c_apps_clk", 0x09b },
	{ "gcc_blsp2_uart1_apps_clk", 0x09c },
	{ "gcc_blsp2_qup2_spi_apps_clk", 0x09e },
	{ "gcc_blsp2_qup2_i2c_apps_clk", 0x0a0 },
	{ "gcc_blsp2_uart2_apps_clk", 0x0a1 },
	{ "gcc_blsp2_qup3_spi_apps_clk", 0x0a3 },
	{ "gcc_blsp2_qup3_i2c_apps_clk", 0x0a4 },
	{ "gcc_blsp2_qup4_spi_apps_clk", 0x0a5 },
	{ "gcc_blsp2_qup4_i2c_apps_clk", 0x0a6 },
	{ "gcc_camss_ahb_clk", 0x0a8 },
	{ "gcc_camss_top_ahb_clk", 0x0a9 },
	{ "gcc_camss_micro_ahb_clk", 0x0aa },
	{ "gcc_camss_gp0_clk", 0x0ab },
	{ "gcc_camss_gp1_clk", 0x0ac },
	{ "gcc_camss_mclk0_clk", 0x0ad },
	{ "gcc_camss_mclk1_clk", 0x0ae },
	{ "gcc_camss_cci_clk", 0x0af },
	{ "gcc_camss_cci_ahb_clk", 0x0b0 },
	{ "gcc_camss_csi0phytimer_clk", 0x0b1 },
	{ "gcc_camss_csi1phytimer_clk", 0x0b2 },
	{ "gcc_camss_jpeg0_clk", 0x0b3 },
	{ "gcc_camss_jpeg_ahb_clk", 0x0b4 },
	{ "gcc_camss_jpeg_axi_clk", 0x0b5 },
	{ "gcc_camss_vfe0_clk", 0x0b8 },
	{ "gcc_camss_cpp_clk", 0x0b9 },
	{ "gcc_camss_cpp_ahb_clk", 0x0ba },
	{ "gcc_camss_vfe_ahb_clk", 0x0bb },
	{ "gcc_camss_vfe_axi_clk", 0x0bc },
	{ "gcc_camss_csi_vfe0_clk", 0x0bf },
	{ "gcc_camss_csi0_clk", 0x0c0 },
	{ "gcc_camss_csi0_ahb_clk", 0x0c1 },
	{ "gcc_camss_csi0phy_clk", 0x0c2 },
	{ "gcc_camss_csi0rdi_clk", 0x0c3 },
	{ "gcc_camss_csi0pix_clk", 0x0c4 },
	{ "gcc_camss_csi1_clk", 0x0c5 },
	{ "gcc_camss_csi1_ahb_clk", 0x0c6 },
	{ "gcc_camss_csi1phy_clk", 0x0c7 },
	{ "gcc_pdm_ahb_clk", 0x0d0 },
	{ "gcc_pdm2_clk", 0x0d2 },
	{ "gcc_prng_ahb_clk", 0x0d8 },
	{ "gcc_mdss_byte1_clk", 0x0da },
	{ "gcc_mdss_esc1_clk", 0x0db },
	{ "gcc_camss_csi0_csiphy_3p_clk", 0x0dc },
	{ "gcc_camss_csi1_csiphy_3p_clk", 0x0dd },
	{ "gcc_camss_csi2_csiphy_3p_clk", 0x0de },
	{ "gcc_camss_csi1rdi_clk", 0x0e0 },
	{ "gcc_camss_csi1pix_clk", 0x0e1 },
	{ "gcc_camss_ispif_ahb_clk", 0x0e2 },
	{ "gcc_camss_csi2_clk", 0x0e3 },
	{ "gcc_camss_csi2_ahb_clk", 0x0e4 },
	{ "gcc_camss_csi2phy_clk", 0x0e5 },
	{ "gcc_camss_csi2rdi_clk", 0x0e6 },
	{ "gcc_camss_csi2pix_clk", 0x0e7 },
	{ "gcc_cpp_tbu_clk", 0x0e9 },
	{ "gcc_rbcpr_gfx_clk", 0x0f0 },
	{ "gcc_boot_rom_ahb_clk", 0x0f8 },
	{ "gcc_crypto_clk", 0x138 },
	{ "gcc_crypto_axi_clk", 0x139 },
	{ "gcc_crypto_ahb_clk", 0x13a },
	{ "gcc_bimc_gpu_clk", 0x157 },
	{ "gcc_apss_ahb_clk", 0x168 },
	{ "gcc_apss_axi_clk", 0x169 },
	{ "gcc_vfe1_tbu_clk", 0x199 },
	{ "gcc_camss_csi_vfe1_clk", 0x1a0 },
	{ "gcc_camss_vfe1_clk", 0x1a1 },
	{ "gcc_camss_vfe1_ahb_clk", 0x1a2 },
	{ "gcc_camss_vfe1_axi_clk", 0x1a3 },
	{ "gcc_camss_cpp_axi_clk", 0x1a4 },
	{ "gcc_venus0_core0_vcodec0_clk", 0x1b8 },
	{ "gcc_camss_mclk2_clk", 0x1bd },
	{ "gcc_camss_mclk3_clk", 0x1bf },
	{ "gcc_oxili_aon_clk", 0x1e8 },
	{ "gcc_oxili_timer_clk", 0x1e9 },
	{ "gcc_oxili_gfx3d_clk", 0x1ea },
	{ "gcc_oxili_ahb_clk", 0x1eb },
	{ "gcc_venus0_vcodec0_clk", 0x1f1 },
	{ "gcc_venus0_axi_clk", 0x1f2 },
	{ "gcc_venus0_ahb_clk", 0x1f3 },
	{ "gcc_mdss_ahb_clk", 0x1f6 },
	{ "gcc_mdss_axi_clk", 0x1f7 },
	{ "gcc_mdss_pclk0_clk", 0x1f8 },
	{ "gcc_mdss_mdp_clk", 0x1f9 },
	{ "gcc_mdss_pclk1_clk", 0x1fa },
	{ "gcc_mdss_vsync_clk", 0x1fb },
	{ "gcc_mdss_byte0_clk", 0x1fc },
	{ "gcc_mdss_esc0_clk", 0x1fd },
	{ "wcnss_m_clk", 0x0ec },
};

static u32 run_measurement(unsigned int ticks)
{
	const u32 ctl_reg = 0x74004;
	const u32 status_reg = 0x74008;
	u32 regval;
	/* Stop counters and set the XO4 counter start value. */
	writel(ticks, gcc_base + ctl_reg);

	/* Wait for timer to become ready. */
	do {
		cpu_relax();
		regval = readl(gcc_base + status_reg);
	} while ((regval & BIT(25)) != 0);

	/* Run measurement and wait for completion. */
	writel(ticks | BIT(20), gcc_base + ctl_reg);
	do {
		cpu_relax();
		regval = readl(gcc_base + status_reg);
	} while ((regval & BIT(25)) == 0);

	/* Return measured ticks. */
	return regval & GENMASK(24, 0);
}

static int msm8953_measure_clock(void *data, u64 *val)
{
	struct debug_mux_item *item = (struct debug_mux_item*) data;
	const u32 plltest_reg = 0x7400C;
	const u32 plltest_val = 0x51A00;
	const u32 xo_div4_cbcr = 0x30034;
	const u32 mux_reg = 0x74000;
	unsigned long flags;
	u32 gcc_xo4_reg, regval;
	u64 raw_count_short, raw_count_full;
	u32 sample_ticks = 0x10000;
	u32 multiplier = item->apcs_mult ?: 1;
	u32 enable_mask = BIT(16);

	if (!gcc_base)
		return -EINVAL;

	if (item->apcs_mult) {
		void __iomem *mux_mem = ioremap(0x0b11101c, 0x4);
		if (IS_ERR_OR_NULL(mux_mem))
			return PTR_ERR(mux_mem);
		writel_relaxed(item->apcs_mux_val << 8, mux_mem);
		iounmap(mux_mem);
	}

	spin_lock_irqsave(&local_clock_reg_lock, flags);

	regval = readl(gcc_base + mux_reg);
	regval = item->value | enable_mask;
	writel(regval, gcc_base + mux_reg);
	mb();
	udelay(1);

	/* Enable CXO/4 and RINGOSC branch. */
	gcc_xo4_reg = readl(gcc_base + xo_div4_cbcr);
	gcc_xo4_reg |= BIT(0); //CBCR_BRANCH_ENABLE_BIT
	writel(gcc_xo4_reg, gcc_base + xo_div4_cbcr);
	mb();

	/*
	 * The ring oscillator counter will not reset if the measured clock
	 * is not running.  To detect this, run a short measurement before
	 * the full measurement.  If the raw results of the two are the same
	 * then the clock must be off.
	 */

	/* Run a short measurement. (~1 ms) */
	raw_count_short = run_measurement(0x1000);
	/* Run a full measurement. (~14 ms) */
	raw_count_full = run_measurement(sample_ticks);

	gcc_xo4_reg &= ~BIT(0); // CBCR_BRANCH_ENABLE_BIT
	writel(gcc_xo4_reg, gcc_base + xo_div4_cbcr);

	/* Return 0 if the clock is off. */
	if (raw_count_full == raw_count_short) {
		*val = 0;
	} else {
		/* Compute rate in Hz. */
		raw_count_full = ((raw_count_full * 10) + 15) * 4800000;
		do_div(raw_count_full, ((sample_ticks * 10) + 35));
		*val = (raw_count_full * multiplier);
	}
	writel(plltest_val, gcc_base + plltest_reg);

	regval = readl(gcc_base + mux_reg);
	/* clear and set post divider bits */
	regval &= ~enable_mask;
	writel(regval, gcc_base + mux_reg);
	spin_unlock_irqrestore(&local_clock_reg_lock, flags);

	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(gcc_debug_fops, msm8953_measure_clock, NULL, "%llu\n");

static int __init msm8953_clkdbg_init(void)
{
	struct device_node *np = of_find_compatible_node(NULL, NULL, "qcom,gcc-msm8953");
	if (!np)
		return -ENODEV;

	of_node_put(np);

	int i = 0;

	gcc_base = ioremap(0x1800000, 0x80000);
	if (!gcc_base)
		return -ENOMEM;

	rootdir = debugfs_create_dir("clk-counter", NULL);
	if (IS_ERR_OR_NULL(rootdir)) {
		iounmap(gcc_base);
		gcc_base = NULL;
		return PTR_ERR(rootdir) ?: -ENODATA;
	}

	for (i = 0; i < ARRAY_SIZE(msm8953_mux_parents); i++) {
		debugfs_create_file(msm8953_mux_parents[i].name, 0400,
				rootdir, &msm8953_mux_parents[i],
				&gcc_debug_fops);
	}

	return 0;
}
module_init(msm8953_clkdbg_init);

static void __exit msm8953_clkdbg_exit(void)
{
	if (!IS_ERR_OR_NULL(rootdir)) {
		debugfs_remove_recursive(rootdir);
		rootdir = NULL;
	}

	if (gcc_base) {
		iounmap(gcc_base);
		gcc_base = NULL;
	}
}
module_exit(msm8953_clkdbg_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Clock rate counter for msm8953");

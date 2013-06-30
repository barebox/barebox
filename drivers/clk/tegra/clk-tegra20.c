/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/lowlevel.h>
#include <mach/tegra20-car.h>

#include "clk.h"

static void __iomem *car_base;

enum tegra20_clks {
	cpu, ac97 = 3, rtc, timer, uarta, uartb, gpio, sdmmc2, i2s1 = 11, i2c1,
	ndflash, sdmmc1, sdmmc4, twc, pwm, i2s2, epp, gr2d = 21, usbd, isp,
	gr3d, ide, disp2, disp1, host1x, vcp, cache2 = 31, mem, ahbdma, apbdma,
	kbc = 36, stat_mon, pmc, fuse, kfuse, sbc1, nor, spi, sbc2, xio, sbc3,
	dvc, dsi, mipi = 50, hdmi, csi, tvdac, i2c2, uartc, emc = 57, usb2,
	usb3, mpe, vde, bsea, bsev, speedo, uartd, uarte, i2c3, sbc4, sdmmc3,
	pex, owr, afi, csite, pcie_xclk, avpucq = 75, la, irama = 84, iramb,
	iramc, iramd, cram2, audio_2x, clk_d, csus = 92, cdev2, cdev1,
	vfir = 96, spdif_in, spdif_out, vi, vi_sensor, tvo, cve,
	osc, clk_32k, clk_m, sclk, cclk, hclk, pclk, blink, pll_a, pll_a_out0,
	pll_c, pll_c_out1, pll_d, pll_d_out0, pll_e, pll_m, pll_m_out1,
	pll_p, pll_p_out1, pll_p_out2, pll_p_out3, pll_p_out4, pll_u,
	pll_x, audio, pll_ref, twd, clk_max,
};

static struct clk *clks[clk_max];
static struct clk_onecell_data clk_data;

static unsigned int get_pll_ref_div(void)
{
	u32 osc_ctrl = readl(car_base + CRC_OSC_CTRL);

	return 1U << ((osc_ctrl & CRC_OSC_CTRL_PLL_REF_DIV_MASK) >>
		      CRC_OSC_CTRL_PLL_REF_DIV_SHIFT);
}

static void tegra20_osc_clk_init(void)
{
	clks[clk_m] = clk_fixed("clk_m", tegra_get_osc_clock());
	clks[clk_32k] = clk_fixed("clk_32k", 32768);

	clks[pll_ref] = clk_fixed_factor("pll_ref", "clk_m", 1,
					 get_pll_ref_div());
}

/* PLL frequency tables */
static struct tegra_clk_pll_freq_table pll_c_freq_table[] = {
	{ 12000000, 600000000, 600, 12, 1, 8 },
	{ 13000000, 600000000, 600, 13, 1, 8 },
	{ 19200000, 600000000, 500, 16, 1, 6 },
	{ 26000000, 600000000, 600, 26, 1, 8 },
	{ 0, 0, 0, 0, 0, 0 },
};

static struct tegra_clk_pll_freq_table pll_p_freq_table[] = {
	{ 12000000, 216000000, 432, 12, 2, 8},
	{ 13000000, 216000000, 432, 13, 2, 8},
	{ 19200000, 216000000, 90,   4, 2, 1},
	{ 26000000, 216000000, 432, 26, 2, 8},
	{ 0, 0, 0, 0, 0, 0 },
};

static struct tegra_clk_pll_freq_table pll_m_freq_table[] = {
	{ 12000000, 666000000, 666, 12, 1, 8},
	{ 13000000, 666000000, 666, 13, 1, 8},
	{ 19200000, 666000000, 555, 16, 1, 8},
	{ 26000000, 666000000, 666, 26, 1, 8},
	{ 12000000, 600000000, 600, 12, 1, 8},
	{ 13000000, 600000000, 600, 13, 1, 8},
	{ 19200000, 600000000, 375, 12, 1, 6},
	{ 26000000, 600000000, 600, 26, 1, 8},
	{ 0, 0, 0, 0, 0, 0 },
};

static struct tegra_clk_pll_freq_table pll_x_freq_table[] = {
	/* 1 GHz */
	{ 12000000, 1000000000, 1000, 12, 1, 12},
	{ 13000000, 1000000000, 1000, 13, 1, 12},
	{ 19200000, 1000000000, 625,  12, 1, 8},
	{ 26000000, 1000000000, 1000, 26, 1, 12},

	/* 912 MHz */
	{ 12000000, 912000000,  912,  12, 1, 12},
	{ 13000000, 912000000,  912,  13, 1, 12},
	{ 19200000, 912000000,  760,  16, 1, 8},
	{ 26000000, 912000000,  912,  26, 1, 12},

	/* 816 MHz */
	{ 12000000, 816000000,  816,  12, 1, 12},
	{ 13000000, 816000000,  816,  13, 1, 12},
	{ 19200000, 816000000,  680,  16, 1, 8},
	{ 26000000, 816000000,  816,  26, 1, 12},

	/* 760 MHz */
	{ 12000000, 760000000,  760,  12, 1, 12},
	{ 13000000, 760000000,  760,  13, 1, 12},
	{ 19200000, 760000000,  950,  24, 1, 8},
	{ 26000000, 760000000,  760,  26, 1, 12},

	/* 750 MHz */
	{ 12000000, 750000000,  750,  12, 1, 12},
	{ 13000000, 750000000,  750,  13, 1, 12},
	{ 19200000, 750000000,  625,  16, 1, 8},
	{ 26000000, 750000000,  750,  26, 1, 12},

	/* 608 MHz */
	{ 12000000, 608000000,  608,  12, 1, 12},
	{ 13000000, 608000000,  608,  13, 1, 12},
	{ 19200000, 608000000,  380,  12, 1, 8},
	{ 26000000, 608000000,  608,  26, 1, 12},

	/* 456 MHz */
	{ 12000000, 456000000,  456,  12, 1, 12},
	{ 13000000, 456000000,  456,  13, 1, 12},
	{ 19200000, 456000000,  380,  16, 1, 8},
	{ 26000000, 456000000,  456,  26, 1, 12},

	/* 312 MHz */
	{ 12000000, 312000000,  312,  12, 1, 12},
	{ 13000000, 312000000,  312,  13, 1, 12},
	{ 19200000, 312000000,  260,  16, 1, 8},
	{ 26000000, 312000000,  312,  26, 1, 12},

	{ 0, 0, 0, 0, 0, 0 },
};

static struct tegra_clk_pll_freq_table pll_u_freq_table[] = {
	{ 12000000, 480000000, 960, 12, 2, 0},
	{ 13000000, 480000000, 960, 13, 2, 0},
	{ 19200000, 480000000, 200, 4,  2, 0},
	{ 26000000, 480000000, 960, 26, 2, 0},
	{ 0, 0, 0, 0, 0, 0 },
};

/* PLL parameters */
static struct tegra_clk_pll_params pll_c_params = {
	.input_min = 2000000,
	.input_max = 31000000,
	.cf_min = 1000000,
	.cf_max = 6000000,
	.vco_min = 20000000,
	.vco_max = 1400000000,
	.base_reg = CRC_PLLC_BASE,
	.misc_reg = CRC_PLLC_MISC,
	.lock_bit_idx = CRC_PLL_BASE_LOCK,
	.lock_enable_bit_idx = CRC_PLL_MISC_LOCK_ENABLE,
	.lock_delay = 300,
};

static struct tegra_clk_pll_params pll_p_params = {
	.input_min = 2000000,
	.input_max = 31000000,
	.cf_min = 1000000,
	.cf_max = 6000000,
	.vco_min = 20000000,
	.vco_max = 1400000000,
	.base_reg = CRC_PLLP_BASE,
	.misc_reg = CRC_PLLP_MISC,
	.lock_bit_idx = CRC_PLL_BASE_LOCK,
	.lock_enable_bit_idx = CRC_PLL_MISC_LOCK_ENABLE,
	.lock_delay = 300,
};

static struct tegra_clk_pll_params pll_m_params = {
	.input_min = 2000000,
	.input_max = 31000000,
	.cf_min = 1000000,
	.cf_max = 6000000,
	.vco_min = 20000000,
	.vco_max = 1200000000,
	.base_reg = CRC_PLLM_BASE,
	.misc_reg = CRC_PLLM_MISC,
	.lock_bit_idx = CRC_PLL_BASE_LOCK,
	.lock_enable_bit_idx = CRC_PLL_MISC_LOCK_ENABLE,
	.lock_delay = 300,
};

static struct tegra_clk_pll_params pll_x_params = {
	.input_min = 2000000,
	.input_max = 31000000,
	.cf_min = 1000000,
	.cf_max = 6000000,
	.vco_min = 20000000,
	.vco_max = 1200000000,
	.base_reg = CRC_PLLX_BASE,
	.misc_reg = CRC_PLLX_MISC,
	.lock_bit_idx = CRC_PLL_BASE_LOCK,
	.lock_enable_bit_idx = CRC_PLL_MISC_LOCK_ENABLE,
	.lock_delay = 300,
};

static struct tegra_clk_pll_params pll_u_params = {
	.input_min = 2000000,
	.input_max = 40000000,
	.cf_min = 1000000,
	.cf_max = 6000000,
	.vco_min = 48000000,
	.vco_max = 960000000,
	.base_reg = CRC_PLLU_BASE,
	.misc_reg = CRC_PLLU_MISC,
	.lock_bit_idx = CRC_PLL_BASE_LOCK,
	.lock_enable_bit_idx = CRC_PLLDU_MISC_LOCK_ENABLE,
	.lock_delay = 1000,
};

static void tegra20_pll_init(void)
{
	/* PLLC */
	clks[pll_c] = tegra_clk_register_pll("pll_c", "pll_ref", car_base,
			0, 0, &pll_c_params, TEGRA_PLL_HAS_CPCON,
			pll_c_freq_table);

	clks[pll_c_out1] = tegra_clk_register_pll_out("pll_c_out1", "pll_c",
			car_base + CRC_PLLC_OUT, 0, TEGRA_DIVIDER_ROUND_UP);

	/* PLLP */
	clks[pll_p] = tegra_clk_register_pll("pll_p", "pll_ref", car_base,
			0, 216000000, &pll_p_params, TEGRA_PLL_FIXED |
			TEGRA_PLL_HAS_CPCON, pll_p_freq_table);

	clks[pll_p_out1] = tegra_clk_register_pll_out("pll_p_out1", "pll_p",
			car_base + CRC_PLLP_OUTA, 0,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	clks[pll_p_out2] = tegra_clk_register_pll_out("pll_p_out2", "pll_p",
			car_base + CRC_PLLP_OUTA, 16,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	clks[pll_p_out3] = tegra_clk_register_pll_out("pll_p_out3", "pll_p",
			car_base + CRC_PLLP_OUTB, 0,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	clks[pll_p_out4] = tegra_clk_register_pll_out("pll_p_out4", "pll_p",
			car_base + CRC_PLLP_OUTB, 16,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	/* PLLM */
	clks[pll_m] = tegra_clk_register_pll("pll_m", "pll_ref", car_base,
			0, 0, &pll_m_params, TEGRA_PLL_HAS_CPCON,
			pll_m_freq_table);

	clks[pll_m_out1] = tegra_clk_register_pll_out("pll_m_out1", "pll_m",
			car_base + CRC_PLLM_OUT, 0, TEGRA_DIVIDER_ROUND_UP);

	/* PLLX */
	clks[pll_x] = tegra_clk_register_pll("pll_x", "pll_ref", car_base,
			0, 0, &pll_x_params, TEGRA_PLL_HAS_CPCON,
			pll_x_freq_table);

	/* PLLU */
	clks[pll_u] = tegra_clk_register_pll("pll_u", "pll_ref", car_base,
			0, 0, &pll_u_params, TEGRA_PLLU |
			TEGRA_PLL_HAS_CPCON, pll_u_freq_table);
}

static const char *mux_pllpcm_clkm[] = {"pll_p", "pll_c", "pll_m", "clk_m"};

static void tegra20_periph_init(void)
{
	/* peripheral clocks without a divider */
	clks[uarta] = tegra_clk_register_periph_nodiv("uarta", mux_pllpcm_clkm,
			ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTA, uarta, TEGRA_PERIPH_ON_APB);
	clks[uartb] = tegra_clk_register_periph_nodiv("uartb", mux_pllpcm_clkm,
			ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTB, uartb, TEGRA_PERIPH_ON_APB);
	clks[uartc] = tegra_clk_register_periph_nodiv("uartc", mux_pllpcm_clkm,
			ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTC, uartc, TEGRA_PERIPH_ON_APB);
	clks[uartd] = tegra_clk_register_periph_nodiv("uartd", mux_pllpcm_clkm,
			ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTD, uartd, TEGRA_PERIPH_ON_APB);
	clks[uarte] = tegra_clk_register_periph_nodiv("uarte", mux_pllpcm_clkm,
			ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTE, uarte, TEGRA_PERIPH_ON_APB);
}

static struct tegra_clk_init_table init_table[] = {
	{pll_p,		clk_max,	216000000,	1},
	{pll_p_out1,	clk_max,	28800000,	1},
	{pll_p_out2,	clk_max,	48000000,	1},
	{pll_p_out3,	clk_max,	72000000,	1},
	{pll_p_out4,	clk_max,	24000000,	1},
	{pll_c,		clk_max,	600000000,	1},
	{pll_c_out1,	clk_max,	120000000,	1},
	{uarta,		pll_p,		0,		1},
	{uartb,		pll_p,		0,		1},
	{uartc,		pll_p,		0,		1},
	{uartd,		pll_p,		0,		1},
	{uarte,		pll_p,		0,		1},
	{clk_max,	clk_max,	0,		0}, /* sentinel */
};

static int tegra20_car_probe(struct device_d *dev)
{
	car_base = dev_request_mem_region(dev, 0);
	if (!car_base)
		return -EBUSY;

	tegra20_osc_clk_init();
	tegra20_pll_init();
	tegra20_periph_init();

	tegra_init_from_table(init_table, clks, clk_max);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get,
			    &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id tegra20_car_dt_ids[] = {
	{
		.compatible = "nvidia,tegra20-car",
	}, {
		/* sentinel */
	}
};

static struct driver_d tegra20_car_driver = {
	.probe	= tegra20_car_probe,
	.name	= "tegra20-car",
	.of_compatible = DRV_OF_COMPAT(tegra20_car_dt_ids),
};

static int tegra20_car_init(void)
{
	return platform_driver_register(&tegra20_car_driver);
}
postcore_initcall(tegra20_car_init);

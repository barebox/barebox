// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <dt-bindings/clock/tegra20-car.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/lowlevel.h>
#include <mach/tegra20-car.h>

#include "clk.h"

static void __iomem *car_base;

static struct clk *clks[TEGRA20_CLK_CLK_MAX];
static struct clk_onecell_data clk_data;

static unsigned int get_pll_ref_div(void)
{
	u32 osc_ctrl = readl(car_base + CRC_OSC_CTRL);

	return 1U << ((osc_ctrl & CRC_OSC_CTRL_PLL_REF_DIV_MASK) >>
		      CRC_OSC_CTRL_PLL_REF_DIV_SHIFT);
}

static void tegra20_osc_clk_init(void)
{
	clks[TEGRA20_CLK_CLK_M] = clk_fixed("clk_m", tegra_get_osc_clock());
	clks[TEGRA20_CLK_CLK_32K] = clk_fixed("clk_32k", 32768);

	clks[TEGRA20_CLK_PLL_REF] = clk_fixed_factor("pll_ref", "clk_m", 1,
						     get_pll_ref_div(), 0);
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
	clks[TEGRA20_CLK_PLL_C] = tegra_clk_register_pll("pll_c", "pll_ref",
			car_base, 0, 0, &pll_c_params, TEGRA_PLL_HAS_CPCON,
			pll_c_freq_table);

	clks[TEGRA20_CLK_PLL_C_OUT1] = tegra_clk_register_pll_out("pll_c_out1",
			"pll_c", car_base + CRC_PLLC_OUT, 0,
			TEGRA_DIVIDER_ROUND_UP);

	/* PLLP */
	clks[TEGRA20_CLK_PLL_P] = tegra_clk_register_pll("pll_p", "pll_ref",
			car_base, 0, 216000000, &pll_p_params, TEGRA_PLL_FIXED |
			TEGRA_PLL_HAS_CPCON, pll_p_freq_table);

	clks[TEGRA20_CLK_PLL_P_OUT1] = tegra_clk_register_pll_out("pll_p_out1",
			"pll_p", car_base + CRC_PLLP_OUTA, 0,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	clks[TEGRA20_CLK_PLL_P_OUT2] = tegra_clk_register_pll_out("pll_p_out2",
			"pll_p", car_base + CRC_PLLP_OUTA, 16,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	clks[TEGRA20_CLK_PLL_P_OUT3] = tegra_clk_register_pll_out("pll_p_out3",
			"pll_p", car_base + CRC_PLLP_OUTB, 0,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	clks[TEGRA20_CLK_PLL_P_OUT4] = tegra_clk_register_pll_out("pll_p_out4",
			"pll_p", car_base + CRC_PLLP_OUTB, 16,
			TEGRA_DIVIDER_FIXED | TEGRA_DIVIDER_ROUND_UP);

	/* PLLM */
	clks[TEGRA20_CLK_PLL_M] = tegra_clk_register_pll("pll_m", "pll_ref",
			car_base, 0, 0, &pll_m_params, TEGRA_PLL_HAS_CPCON,
			pll_m_freq_table);

	clks[TEGRA20_CLK_PLL_M_OUT1] = tegra_clk_register_pll_out("pll_m_out1",
			"pll_m", car_base + CRC_PLLM_OUT, 0,
			TEGRA_DIVIDER_ROUND_UP);

	/* PLLX */
	clks[TEGRA20_CLK_PLL_X] = tegra_clk_register_pll("pll_x", "pll_ref",
			car_base, 0, 0, &pll_x_params, TEGRA_PLL_HAS_CPCON,
			pll_x_freq_table);

	/* PLLU */
	clks[TEGRA20_CLK_PLL_U] = tegra_clk_register_pll("pll_u", "pll_ref",
			car_base, 0, 0, &pll_u_params, TEGRA_PLLU |
			TEGRA_PLL_HAS_CPCON, pll_u_freq_table);
}

static const char *mux_pllpcm_clkm[] = {"pll_p", "pll_c", "pll_m", "clk_m"};

static void tegra20_periph_init(void)
{
	/* peripheral clocks without a divider */
	clks[TEGRA20_CLK_UARTA] = tegra_clk_register_periph_nodiv("uarta",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTA, TEGRA20_CLK_UARTA,
			TEGRA_PERIPH_ON_APB);
	clks[TEGRA20_CLK_UARTB] = tegra_clk_register_periph_nodiv("uartb",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTB, 7,
			TEGRA_PERIPH_ON_APB);
	clks[TEGRA20_CLK_UARTC] = tegra_clk_register_periph_nodiv("uartc",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTC, TEGRA20_CLK_UARTC,
			TEGRA_PERIPH_ON_APB);
	clks[TEGRA20_CLK_UARTD] = tegra_clk_register_periph_nodiv("uartd",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTD, TEGRA20_CLK_UARTD,
			TEGRA_PERIPH_ON_APB);
	clks[TEGRA20_CLK_UARTE] = tegra_clk_register_periph_nodiv("uarte",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_UARTE, TEGRA20_CLK_UARTE,
			TEGRA_PERIPH_ON_APB);

	/* peripheral clocks with a divider */
	clks[TEGRA20_CLK_SDMMC1] = tegra_clk_register_periph("sdmmc1",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_SDMMC1, TEGRA20_CLK_SDMMC1, 1);
	clks[TEGRA20_CLK_SDMMC2] = tegra_clk_register_periph("sdmmc2",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_SDMMC2, TEGRA20_CLK_SDMMC2, 1);
	clks[TEGRA20_CLK_SDMMC3] = tegra_clk_register_periph("sdmmc3",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_SDMMC3, TEGRA20_CLK_SDMMC3, 1);
	clks[TEGRA20_CLK_SDMMC4] = tegra_clk_register_periph("sdmmc4",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_SDMMC4, TEGRA20_CLK_SDMMC4, 1);

	clks[TEGRA20_CLK_I2C1] = tegra_clk_register_periph_div16("i2c1",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_I2C1, TEGRA20_CLK_I2C1, 1);
	clks[TEGRA20_CLK_I2C2] = tegra_clk_register_periph_div16("i2c2",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_I2C2, TEGRA20_CLK_I2C2, 1);
	clks[TEGRA20_CLK_I2C3] = tegra_clk_register_periph_div16("i2c3",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_I2C3, TEGRA20_CLK_I2C3, 1);
	clks[TEGRA20_CLK_DVC] = tegra_clk_register_periph_div16("dvc",
			mux_pllpcm_clkm, ARRAY_SIZE(mux_pllpcm_clkm), car_base,
			CRC_CLK_SOURCE_DVC, TEGRA20_CLK_DVC, 1);
}

static struct tegra_clk_init_table init_table[] = {
	{TEGRA20_CLK_PLL_P,		TEGRA20_CLK_CLK_MAX,	216000000,	1},
	{TEGRA20_CLK_PLL_P_OUT1,	TEGRA20_CLK_CLK_MAX,	28800000,	1},
	{TEGRA20_CLK_PLL_P_OUT2,	TEGRA20_CLK_CLK_MAX,	48000000,	1},
	{TEGRA20_CLK_PLL_P_OUT3,	TEGRA20_CLK_CLK_MAX,	72000000,	1},
	{TEGRA20_CLK_PLL_P_OUT4,	TEGRA20_CLK_CLK_MAX,	24000000,	1},
	{TEGRA20_CLK_PLL_C,		TEGRA20_CLK_CLK_MAX,	600000000,	1},
	{TEGRA20_CLK_PLL_C_OUT1,	TEGRA20_CLK_CLK_MAX,	120000000,	1},
	{TEGRA20_CLK_UARTA,		TEGRA20_CLK_PLL_P,	0,		0},
	{TEGRA20_CLK_UARTB,		TEGRA20_CLK_PLL_P,	0,		0},
	{TEGRA20_CLK_UARTC,		TEGRA20_CLK_PLL_P,	0,		0},
	{TEGRA20_CLK_UARTD,		TEGRA20_CLK_PLL_P,	0,		0},
	{TEGRA20_CLK_UARTE,		TEGRA20_CLK_PLL_P,	0,		0},
	{TEGRA20_CLK_SDMMC1,		TEGRA20_CLK_PLL_P,	48000000,	0},
	{TEGRA20_CLK_SDMMC2,		TEGRA20_CLK_PLL_P,	48000000,	0},
	{TEGRA20_CLK_SDMMC3,		TEGRA20_CLK_PLL_P,	48000000,	0},
	{TEGRA20_CLK_SDMMC4,		TEGRA20_CLK_PLL_P,	48000000,	0},
	{TEGRA20_CLK_CLK_MAX,		TEGRA20_CLK_CLK_MAX,	0,	0}, /* sentinel */
};

static int tegra20_car_probe(struct device_d *dev)
{
	struct resource *iores;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	car_base = IOMEM(iores->start);

	tegra20_osc_clk_init();
	tegra20_pll_init();
	tegra20_periph_init();

	tegra_init_from_table(init_table, clks, TEGRA20_CLK_CLK_MAX);

	/* speed up system bus */
	writel(CRC_SCLK_BURST_POLICY_SYS_STATE_RUN <<
	       CRC_SCLK_BURST_POLICY_SYS_STATE_SHIFT |
	       CRC_SCLK_BURST_POLICY_SRC_PLLC_OUT1 <<
	       CRC_SCLK_BURST_POLICY_RUN_SRC_SHIFT,
	       car_base + CRC_SCLK_BURST_POLICY);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get,
			    &clk_data);

	tegra_clk_init_rst_controller(car_base, dev->device_node, 3 * 32);
	tegra_clk_reset_uarts();

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

postcore_platform_driver(tegra20_car_driver);

/*
 * Marvell Armada 380/385 SoC clocks
 *
 * Copyright (C) 2014 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 * Andrew Lunn <andrew@lunn.ch>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <io.h>

#include "common.h"

/*
 * Core Clocks
 *
 * Armada XP Sample At Reset is a 64 bit bitfiled split in two
 * register of 32 bits
 */

#define SAR_A380_TCLK_FREQ_OPT			15
#define SAR_A380_TCLK_FREQ_OPT_MASK		0x1
#define SAR_A380_CPU_DDR_L2_FREQ_OPT		10
#define SAR_A380_CPU_DDR_L2_FREQ_OPT_MASK	0x1f

/* Armada XP TCLK frequency is fixed to 250MHz */
static u32 a38x_get_tclk_freq(void __iomem *sar)
{
	if ((readl(sar) >> SAR_A380_TCLK_FREQ_OPT) & SAR_A380_TCLK_FREQ_OPT_MASK)
		return 200000000;
	else
		return 250000000;
}

static const u32 a38x_cpu_freqs[] = {
	0, 0, 0, 0,
	1666000000, 0, 0, 0,
	1332000000, 0, 0, 0,
	1600000000,
};

static u32 a38x_get_cpu_freq(void __iomem *sar)
{
	u32 cpu_freq_select = ((readl(sar) >> SAR_A380_CPU_DDR_L2_FREQ_OPT) &
			   SAR_A380_CPU_DDR_L2_FREQ_OPT_MASK);

	if (cpu_freq_select >= ARRAY_SIZE(a38x_cpu_freqs)) {
		pr_err("CPU freq select unsupported: %d\n", cpu_freq_select);
		return 0;
	}

	return a38x_cpu_freqs[cpu_freq_select];
}

enum { A380_CPU_TO_DDR, A380_CPU_TO_L2 };

static const struct coreclk_ratio a38x_coreclk_ratios[] = {
	{ .id = A380_CPU_TO_L2,  .name = "l2clk" },
	{ .id = A380_CPU_TO_DDR, .name = "ddrclk" },
};

static const int armada_38x_cpu_l2_ratios[32][2] = {
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {1, 2}, {0, 1}, {0, 1}, {0, 1},
        {1, 2}, {0, 1}, {0, 1}, {0, 1},
        {1, 2}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int armada_38x_cpu_ddr_ratios[32][2] = {
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {1, 2}, {0, 1}, {0, 1}, {0, 1},
        {1, 2}, {0, 1}, {0, 1}, {0, 1},
        {1, 2}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
        {0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static void a38x_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	u32 opt = ((readl(sar) >> SAR_A380_CPU_DDR_L2_FREQ_OPT) &
	      SAR_A380_CPU_DDR_L2_FREQ_OPT_MASK);

	switch (id) {
	case A380_CPU_TO_L2:
		*mult = armada_38x_cpu_l2_ratios[opt][0];
		*div = armada_38x_cpu_l2_ratios[opt][1];
		break;
	case A380_CPU_TO_DDR:
		*mult = armada_38x_cpu_ddr_ratios[opt][0];
		*div = armada_38x_cpu_ddr_ratios[opt][1];
		break;
	}
}

const struct coreclk_soc_desc armada_38x_coreclks = {
	.get_tclk_freq = a38x_get_tclk_freq,
	.get_cpu_freq = a38x_get_cpu_freq,
	.get_clk_ratio = a38x_get_clk_ratio,
	.ratios = a38x_coreclk_ratios,
	.num_ratios = ARRAY_SIZE(a38x_coreclk_ratios),
};

/*
 * Clock Gating Control
 */
const struct clk_gating_soc_desc armada_38x_gating_desc[] = {
	{ "audio", NULL, 0 },
	{ "ge2", NULL,  2 },
	{ "ge1", NULL, 3 },
	{ "ge0", NULL, 4 },
	{ "pex1", NULL, 5 },
	{ "pex2", NULL, 6 },
	{ "pex3", NULL, 7 },
	{ "pex0", NULL, 8 },
	{ "usb3h0", NULL, 9 },
	{ "usb3h1", NULL, 10 },
	{ "usb3d", NULL, 11 },
	{ "bm", NULL, 13 },
	{ "crypto0z", NULL, 14 },
	{ "sata0", NULL, 15 },
	{ "crypto1z", NULL, 16 },
	{ "sdio", NULL, 17 },
	{ "usb2", NULL, 18 },
	{ "crypto1", NULL, 21 },
	{ "xor0", NULL, 22 },
	{ "crypto0", NULL, 23 },
	{ "tdm", NULL, 25 },
	{ "xor1", NULL, 28 },
	{ "sata1", NULL, 30 },
	{ }
};

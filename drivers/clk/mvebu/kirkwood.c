/*
 * Marvell Kirkwood SoC clocks
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux Marvell MVEBU clock providers
 *   Copyright (C) 2012 Marvell
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
 * Kirkwood PLL sample-at-reset configuration
 * (6180 has different SAR layout than other Kirkwood SoCs)
 *
 * SAR0[4:3,22,1] : CPU frequency (6281,6292,6282)
 *	4  =  600 MHz
 *	6  =  800 MHz
 *	7  = 1000 MHz
 *	9  = 1200 MHz
 *	12 = 1500 MHz
 *	13 = 1600 MHz
 *	14 = 1800 MHz
 *	15 = 2000 MHz
 *	others reserved.
 *
 * SAR0[19,10:9] : CPU to L2 Clock divider ratio (6281,6292,6282)
 *	1 = (1/2) * CPU
 *	3 = (1/3) * CPU
 *	5 = (1/4) * CPU
 *	others reserved.
 *
 * SAR0[8:5] : CPU to DDR DRAM Clock divider ratio (6281,6292,6282)
 *	2 = (1/2) * CPU
 *	4 = (1/3) * CPU
 *	6 = (1/4) * CPU
 *	7 = (2/9) * CPU
 *	8 = (1/5) * CPU
 *	9 = (1/6) * CPU
 *	others reserved.
 *
 * SAR0[4:2] : Kirkwood 6180 cpu/l2/ddr clock configuration (6180 only)
 *	5 = [CPU =  600 MHz, L2 = (1/2) * CPU, DDR = 200 MHz = (1/3) * CPU]
 *	6 = [CPU =  800 MHz, L2 = (1/2) * CPU, DDR = 200 MHz = (1/4) * CPU]
 *	7 = [CPU = 1000 MHz, L2 = (1/2) * CPU, DDR = 200 MHz = (1/5) * CPU]
 *	others reserved.
 *
 * SAR0[21] : TCLK frequency
 *	0 = 200 MHz
 *	1 = 166 MHz
 *	others reserved.
 */

#define SAR_KIRKWOOD_CPU_FREQ(x)	\
	(((x & (1 <<  1)) >>  1) |	\
	 ((x & (1 << 22)) >> 21) |	\
	 ((x & (3 <<  3)) >>  1))
#define SAR_KIRKWOOD_L2_RATIO(x)	\
	(((x & (3 <<  9)) >> 9) |	\
	 (((x & (1 << 19)) >> 17)))
#define SAR_KIRKWOOD_DDR_RATIO		5
#define SAR_KIRKWOOD_DDR_RATIO_MASK	0xf
#define SAR_MV88F6180_CLK		2
#define SAR_MV88F6180_CLK_MASK		0x7
#define SAR_KIRKWOOD_TCLK_FREQ		21
#define SAR_KIRKWOOD_TCLK_FREQ_MASK	0x1

enum { KIRKWOOD_CPU_TO_L2, KIRKWOOD_CPU_TO_DDR };

static const struct coreclk_ratio kirkwood_coreclk_ratios[] = {
	{ .id = KIRKWOOD_CPU_TO_L2, .name = "l2clk", },
	{ .id = KIRKWOOD_CPU_TO_DDR, .name = "ddrclk", }
};

static u32 kirkwood_get_tclk_freq(void __iomem *sar)
{
	u32 opt = (readl(sar) >> SAR_KIRKWOOD_TCLK_FREQ) &
		SAR_KIRKWOOD_TCLK_FREQ_MASK;
	return (opt) ? 166666667 : 200000000;
}

static const u32 kirkwood_cpu_freqs[] = {
	0, 0, 0, 0,
	600000000,
	0,
	800000000,
	1000000000,
	0,
	1200000000,
	0, 0,
	1500000000,
	1600000000,
	1800000000,
	2000000000
};

static u32 kirkwood_get_cpu_freq(void __iomem *sar)
{
	u32 opt = SAR_KIRKWOOD_CPU_FREQ(readl(sar));
	return kirkwood_cpu_freqs[opt];
}

static const int kirkwood_cpu_l2_ratios[8][2] = {
	{ 0, 1 }, { 1, 2 }, { 0, 1 }, { 1, 3 },
	{ 0, 1 }, { 1, 4 }, { 0, 1 }, { 0, 1 }
};

static const int kirkwood_cpu_ddr_ratios[16][2] = {
	{ 0, 1 }, { 0, 1 }, { 1, 2 }, { 0, 1 },
	{ 1, 3 }, { 0, 1 }, { 1, 4 }, { 2, 9 },
	{ 1, 5 }, { 1, 6 }, { 0, 1 }, { 0, 1 },
	{ 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }
};

static void kirkwood_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	switch (id) {
	case KIRKWOOD_CPU_TO_L2:
	{
		u32 opt = SAR_KIRKWOOD_L2_RATIO(readl(sar));
		*mult = kirkwood_cpu_l2_ratios[opt][0];
		*div = kirkwood_cpu_l2_ratios[opt][1];
		break;
	}
	case KIRKWOOD_CPU_TO_DDR:
	{
		u32 opt = (readl(sar) >> SAR_KIRKWOOD_DDR_RATIO) &
			SAR_KIRKWOOD_DDR_RATIO_MASK;
		*mult = kirkwood_cpu_ddr_ratios[opt][0];
		*div = kirkwood_cpu_ddr_ratios[opt][1];
		break;
	}
	}
}

static const u32 mv88f6180_cpu_freqs[] = {
	0, 0, 0, 0, 0,
	600000000,
	800000000,
	1000000000
};

static u32 mv88f6180_get_cpu_freq(void __iomem *sar)
{
	u32 opt = (readl(sar) >> SAR_MV88F6180_CLK) & SAR_MV88F6180_CLK_MASK;
	return mv88f6180_cpu_freqs[opt];
}

static const int mv88f6180_cpu_ddr_ratios[8][2] = {
	{ 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 },
	{ 0, 1 }, { 1, 3 }, { 1, 4 }, { 1, 5 }
};

static void mv88f6180_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	switch (id) {
	case KIRKWOOD_CPU_TO_L2:
	{
		/* mv88f6180 has a fixed 1:2 CPU-to-L2 ratio */
		*mult = 1;
		*div = 2;
		break;
	}
	case KIRKWOOD_CPU_TO_DDR:
	{
		u32 opt = (readl(sar) >> SAR_MV88F6180_CLK) &
			SAR_MV88F6180_CLK_MASK;
		*mult = mv88f6180_cpu_ddr_ratios[opt][0];
		*div = mv88f6180_cpu_ddr_ratios[opt][1];
		break;
	}
	}
}

const struct coreclk_soc_desc kirkwood_coreclks = {
	.get_tclk_freq = kirkwood_get_tclk_freq,
	.get_cpu_freq = kirkwood_get_cpu_freq,
	.get_clk_ratio = kirkwood_get_clk_ratio,
	.ratios = kirkwood_coreclk_ratios,
	.num_ratios = ARRAY_SIZE(kirkwood_coreclk_ratios),
};

const struct coreclk_soc_desc mv88f6180_coreclks = {
	.get_tclk_freq = kirkwood_get_tclk_freq,
	.get_cpu_freq = mv88f6180_get_cpu_freq,
	.get_clk_ratio = mv88f6180_get_clk_ratio,
	.ratios = kirkwood_coreclk_ratios,
	.num_ratios = ARRAY_SIZE(kirkwood_coreclk_ratios),
};

/*
 * Clock Gating Control
 */

const struct clk_gating_soc_desc kirkwood_gating_desc[] = {
	{ "ge0", NULL, 0 },
	{ "pex0", NULL, 2 },
	{ "usb0", NULL, 3 },
	{ "sdio", NULL, 4 },
	{ "tsu", NULL, 5 },
	{ "runit", NULL, 7 },
	{ "xor0", NULL, 8 },
	{ "audio", NULL, 9 },
	{ "powersave", "cpuclk", 11 },
	{ "sata0", NULL, 14 },
	{ "sata1", NULL, 15 },
	{ "xor1", NULL, 16 },
	{ "crypto", NULL, 17 },
	{ "pex1", NULL, 18 },
	{ "ge1", NULL, 19 },
	{ "tdm", NULL, 20 },
	{ }
};

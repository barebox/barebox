/*
 * Marvell Armada XP SoC clocks
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
 * Armada XP Sample At Reset is a 64 bit bitfiled split in two
 * register of 32 bits
 */

#define SARL				0	/* Low part [0:31] */
#define	 SARL_AXP_PCLK_FREQ_OPT		21
#define	 SARL_AXP_PCLK_FREQ_OPT_MASK	0x7
#define	 SARL_AXP_FAB_FREQ_OPT		24
#define	 SARL_AXP_FAB_FREQ_OPT_MASK	0xF
#define SARH				4	/* High part [32:63] */
#define	 SARH_AXP_PCLK_FREQ_OPT		(52-32)
#define	 SARH_AXP_PCLK_FREQ_OPT_MASK	0x1
#define	 SARH_AXP_PCLK_FREQ_OPT_SHIFT	3
#define	 SARH_AXP_FAB_FREQ_OPT		(51-32)
#define	 SARH_AXP_FAB_FREQ_OPT_MASK	0x1
#define	 SARH_AXP_FAB_FREQ_OPT_SHIFT	4

enum { AXP_CPU_TO_NBCLK, AXP_CPU_TO_HCLK, AXP_CPU_TO_DRAMCLK };

static const struct coreclk_ratio axp_coreclk_ratios[] = {
	{ .id = AXP_CPU_TO_NBCLK, .name = "nbclk" },
	{ .id = AXP_CPU_TO_HCLK, .name = "hclk" },
	{ .id = AXP_CPU_TO_DRAMCLK, .name = "dramclk" },
};

/* Armada XP TCLK frequency is fixed to 250MHz */
static u32 axp_get_tclk_freq(void __iomem *sar)
{
	return 250000000;
}

static const u32 axp_cpu_freqs[] = {
	1000000000,
	1066000000,
	1200000000,
	1333000000,
	1500000000,
	1666000000,
	1800000000,
	2000000000,
	667000000,
	0,
	800000000,
	1600000000,
};

static u32 axp_get_cpu_freq(void __iomem *sar)
{
	u32 cpu_freq;
	u8 cpu_freq_select = 0;

	cpu_freq_select = ((readl(sar + SARL) >> SARL_AXP_PCLK_FREQ_OPT) &
			   SARL_AXP_PCLK_FREQ_OPT_MASK);
	/*
	 * The upper bit is not contiguous to the other ones and
	 * located in the high part of the SAR registers
	 */
	cpu_freq_select |= (((readl(sar + SARH) >> SARH_AXP_PCLK_FREQ_OPT) &
	     SARH_AXP_PCLK_FREQ_OPT_MASK) << SARH_AXP_PCLK_FREQ_OPT_SHIFT);
	if (cpu_freq_select >= ARRAY_SIZE(axp_cpu_freqs)) {
		pr_err("CPU freq select unsupported: %d\n", cpu_freq_select);
		cpu_freq = 0;
	} else
		cpu_freq = axp_cpu_freqs[cpu_freq_select];

	return cpu_freq;
}

static const int axp_nbclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 2}, {2, 2},
	{1, 2}, {1, 2}, {1, 1}, {2, 3},
	{0, 1}, {1, 2}, {2, 4}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {2, 2},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int axp_hclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 6}, {2, 3},
	{1, 3}, {1, 4}, {1, 2}, {2, 6},
	{0, 1}, {1, 6}, {2, 10}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 2},
	{2, 6}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int axp_dramclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 3}, {2, 3},
	{1, 3}, {1, 2}, {1, 2}, {2, 6},
	{0, 1}, {1, 3}, {2, 5}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static void axp_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	u32 opt = ((readl(sar + SARL) >> SARL_AXP_FAB_FREQ_OPT) &
	      SARL_AXP_FAB_FREQ_OPT_MASK);
	/*
	 * The upper bit is not contiguous to the other ones and
	 * located in the high part of the SAR registers
	 */
	opt |= (((readl(sar + SARH) >> SARH_AXP_FAB_FREQ_OPT) &
		 SARH_AXP_FAB_FREQ_OPT_MASK) << SARH_AXP_FAB_FREQ_OPT_SHIFT);

	switch (id) {
	case AXP_CPU_TO_NBCLK:
		*mult = axp_nbclk_ratios[opt][0];
		*div = axp_nbclk_ratios[opt][1];
		break;
	case AXP_CPU_TO_HCLK:
		*mult = axp_hclk_ratios[opt][0];
		*div = axp_hclk_ratios[opt][1];
		break;
	case AXP_CPU_TO_DRAMCLK:
		*mult = axp_dramclk_ratios[opt][0];
		*div = axp_dramclk_ratios[opt][1];
		break;
	}
}

const struct coreclk_soc_desc armada_xp_coreclks = {
	.get_tclk_freq = axp_get_tclk_freq,
	.get_cpu_freq = axp_get_cpu_freq,
	.get_clk_ratio = axp_get_clk_ratio,
	.ratios = axp_coreclk_ratios,
	.num_ratios = ARRAY_SIZE(axp_coreclk_ratios),
};

/*
 * Clock Gating Control
 */

const struct clk_gating_soc_desc armada_xp_gating_desc[] = {
	{ "audio", NULL, 0 },
	{ "ge3", NULL, 1 },
	{ "ge2", NULL,  2 },
	{ "ge1", NULL, 3 },
	{ "ge0", NULL, 4 },
	{ "pex00", NULL, 5 },
	{ "pex01", NULL, 6 },
	{ "pex02", NULL, 7 },
	{ "pex03", NULL, 8 },
	{ "pex10", NULL, 9 },
	{ "pex11", NULL, 10 },
	{ "pex12", NULL, 11 },
	{ "pex13", NULL, 12 },
	{ "bp", NULL, 13 },
	{ "sata0lnk", NULL, 14 },
	{ "sata0", "sata0lnk", 15 },
	{ "lcd", NULL, 16 },
	{ "sdio", NULL, 17 },
	{ "usb0", NULL, 18 },
	{ "usb1", NULL, 19 },
	{ "usb2", NULL, 20 },
	{ "xor0", NULL, 22 },
	{ "crypto", NULL, 23 },
	{ "tdm", NULL, 25 },
	{ "pex20", NULL, 26 },
	{ "pex30", NULL, 27 },
	{ "xor1", NULL, 28 },
	{ "sata1lnk", NULL, 29 },
	{ "sata1", "sata1lnk", 30 },
	{ }
};

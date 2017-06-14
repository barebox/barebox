/*
 * Marvell EBU SoC common clock handling
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

#ifndef __CLK_MVEBU_COMMON_H_
#define __CLK_MVEBU_COMMON_H_

struct coreclk_ratio {
	int id;
	const char *name;
};

struct coreclk_soc_desc {
	u32 (*get_tclk_freq)(void __iomem *sar);
	u32 (*get_cpu_freq)(void __iomem *sar);
	void (*get_clk_ratio)(void __iomem *sar, int id, int *mult, int *div);
	const struct coreclk_ratio *ratios;
	int num_ratios;
};

struct clk_gating_soc_desc {
	const char *name;
	const char *parent;
	int bit_idx;
};

#ifdef CONFIG_ARCH_ARMADA_370
extern const struct coreclk_soc_desc armada_370_coreclks;
extern const struct clk_gating_soc_desc armada_370_gating_desc[];
#else
static const u32 armada_370_coreclks;
static const u32 armada_370_gating_desc;
#endif

#ifdef CONFIG_ARCH_ARMADA_XP
extern const struct coreclk_soc_desc armada_xp_coreclks;
extern const struct clk_gating_soc_desc armada_xp_gating_desc[];
#else
static const u32 armada_xp_coreclks;
static const u32 armada_xp_gating_desc;
#endif

#ifdef CONFIG_ARCH_ARMADA_38X
extern const struct coreclk_soc_desc armada_38x_coreclks;
extern const struct clk_gating_soc_desc armada_38x_gating_desc[];
#else
static const u32 armada_38x_coreclks;
static const u32 armada_38x_gating_desc;
#endif

#ifdef CONFIG_ARCH_DOVE
extern const struct coreclk_soc_desc dove_coreclks;
extern const struct clk_gating_soc_desc dove_gating_desc[];
#else
static const u32 dove_coreclks;
static const u32 dove_gating_desc;
#endif

#ifdef CONFIG_ARCH_KIRKWOOD
extern const struct coreclk_soc_desc kirkwood_coreclks;
extern const struct coreclk_soc_desc mv88f6180_coreclks;
extern const struct clk_gating_soc_desc kirkwood_gating_desc[];
#else
static const u32 kirkwood_coreclks;
static const u32 mv88f6180_coreclks;
static const u32 kirkwood_gating_desc;
#endif

#endif

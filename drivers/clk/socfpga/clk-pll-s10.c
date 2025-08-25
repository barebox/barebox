// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017, Intel Corporation
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "stratix10-clk.h"
#include "clk.h"

/* Clock Manager offsets */
#define CLK_MGR_PLL_CLK_SRC_SHIFT	16
#define CLK_MGR_PLL_CLK_SRC_MASK	0x3

/* PLL Clock enable bits */
#define SOCFPGA_PLL_POWER		0
#define SOCFPGA_PLL_RESET_MASK		0x2
#define SOCFPGA_PLL_REFDIV_MASK		0x00003F00
#define SOCFPGA_PLL_REFDIV_SHIFT	8
#define SOCFPGA_PLL_AREFDIV_MASK	0x00000F00
#define SOCFPGA_PLL_DREFDIV_MASK	0x00003000
#define SOCFPGA_PLL_DREFDIV_SHIFT	12
#define SOCFPGA_PLL_MDIV_MASK		0xFF000000
#define SOCFPGA_PLL_MDIV_SHIFT		24
#define SOCFPGA_AGILEX_PLL_MDIV_MASK	0x000003FF
#define SWCTRLBTCLKSEL_MASK		0x200
#define SWCTRLBTCLKSEL_SHIFT		9

#define SOCFPGA_BOOT_CLK		"boot_clk"

#define to_socfpga_clk(p) container_of(p, struct socfpga_pll, hw)

static unsigned long agilex_clk_pll_recalc_rate(struct clk_hw *hwclk,
						unsigned long parent_rate)
{
	struct socfpga_pll *socfpgaclk = to_socfpga_clk(hwclk);
	unsigned long arefdiv, reg, mdiv;
	unsigned long long vco_freq;

	/* read VCO1 reg for numerator and denominator */
	reg = readl(socfpgaclk->reg);
	arefdiv = (reg & SOCFPGA_PLL_AREFDIV_MASK) >> SOCFPGA_PLL_REFDIV_SHIFT;

	vco_freq = (unsigned long long)parent_rate / arefdiv;

	/* Read mdiv and fdiv from the fdbck register */
	reg = readl(socfpgaclk->reg + 0x24);
	mdiv = reg & SOCFPGA_AGILEX_PLL_MDIV_MASK;

	vco_freq = (unsigned long long)vco_freq * mdiv;
	return (unsigned long)vco_freq;
}

static unsigned long clk_boot_clk_recalc_rate(struct clk_hw *hwclk,
					 unsigned long parent_rate)
{
	struct socfpga_pll *socfpgaclk = to_socfpga_clk(hwclk);
	u32 div;

	div = ((readl(socfpgaclk->reg) &
		SWCTRLBTCLKSEL_MASK) >>
		SWCTRLBTCLKSEL_SHIFT);
	div += 1;
	return parent_rate / div;
}

static int clk_pll_get_parent(struct clk_hw *hwclk)
{
	struct socfpga_pll *socfpgaclk = to_socfpga_clk(hwclk);
	u32 pll_src;

	pll_src = readl(socfpgaclk->reg);
	return (pll_src >> CLK_MGR_PLL_CLK_SRC_SHIFT) &
		CLK_MGR_PLL_CLK_SRC_MASK;
}

static int clk_boot_get_parent(struct clk_hw *hwclk)
{
	struct socfpga_pll *socfpgaclk = to_socfpga_clk(hwclk);
	u32 pll_src;

	pll_src = readl(socfpgaclk->reg);
	return (pll_src >> SWCTRLBTCLKSEL_SHIFT) &
		SWCTRLBTCLKSEL_MASK;
}

/* TODO need to fix, Agilex5 SM requires change */
static const struct clk_ops agilex5_clk_pll_ops = {
	/* TODO This may require a custom Agilex5 implementation */
	.recalc_rate = agilex_clk_pll_recalc_rate,
	.get_parent = clk_pll_get_parent,
};

static const struct clk_ops clk_boot_ops = {
	.recalc_rate = clk_boot_clk_recalc_rate,
	.get_parent = clk_boot_get_parent,
};

struct clk_hw *agilex5_register_pll(const struct stratix10_pll_clock *clks,
				void __iomem *reg)
{
	struct clk_hw *hw_clk;
	struct socfpga_pll *pll_clk;
	struct clk_init_data init;
	const char *name = clks->name;
	int ret;

	pll_clk = xzalloc(sizeof(*pll_clk));
	pll_clk->reg = reg + clks->offset;

	if (streq(name, SOCFPGA_BOOT_CLK))
		init.ops = &clk_boot_ops;
	else
		init.ops = &agilex5_clk_pll_ops;

	init.name = name;
	init.flags = clks->flags;

	init.num_parents = clks->num_parents;
	init.parent_names = NULL;
	init.parent_data = clks->parent_data;
	pll_clk->hw.init = &init;

	pll_clk->bit_idx = SOCFPGA_PLL_POWER;
	hw_clk = &pll_clk->hw;

	ret = clk_hw_register(NULL, hw_clk);
	if (ret) {
		kfree(pll_clk);
		return ERR_PTR(ret);
	}
	return hw_clk;
}


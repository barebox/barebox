// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clk.h"

#define pll_out_enb(p) (BIT(p->enb_bit_idx))
#define pll_out_rst(p) (BIT(p->rst_bit_idx))

#define to_clk_pll_out(_hw) container_of(_hw, struct tegra_clk_pll_out, hw)

static int clk_pll_out_is_enabled(struct clk_hw *hw)
{
	struct tegra_clk_pll_out *pll_out = to_clk_pll_out(hw);
	u32 val = readl(pll_out->reg);
	int state;

	state = (val & pll_out_enb(pll_out)) ? 1 : 0;
	if (!(val & (pll_out_rst(pll_out))))
		state = 0;
	return state;
}

static int clk_pll_out_enable(struct clk_hw *hw)
{
	struct tegra_clk_pll_out *pll_out = to_clk_pll_out(hw);
	u32 val;

	val = readl(pll_out->reg);

	val |= (pll_out_enb(pll_out) | pll_out_rst(pll_out));

	writel(val, pll_out->reg);
	udelay(2);

	return 0;
}

static void clk_pll_out_disable(struct clk_hw *hw)
{
	struct tegra_clk_pll_out *pll_out = to_clk_pll_out(hw);
	u32 val;

	val = readl(pll_out->reg);

	val &= ~(pll_out_enb(pll_out) | pll_out_rst(pll_out));

	writel(val, pll_out->reg);
	udelay(2);
}

static unsigned long clk_pll_out_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	struct tegra_clk_pll_out *pll_out = to_clk_pll_out(hw);

	return pll_out->div->ops->recalc_rate(clk_to_clk_hw(pll_out->div), parent_rate);
}

static long clk_pll_out_round_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long *prate)
{
	struct tegra_clk_pll_out *pll_out = to_clk_pll_out(hw);

	return pll_out->div->ops->round_rate(clk_to_clk_hw(pll_out->div), rate, prate);
}

static int clk_pll_out_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct tegra_clk_pll_out *pll_out = to_clk_pll_out(hw);

	return pll_out->div->ops->set_rate(clk_to_clk_hw(pll_out->div), rate, parent_rate);
}

const struct clk_ops tegra_clk_pll_out_ops = {
	.is_enabled = clk_pll_out_is_enabled,
	.enable = clk_pll_out_enable,
	.disable = clk_pll_out_disable,
	.recalc_rate = clk_pll_out_recalc_rate,
	.round_rate = clk_pll_out_round_rate,
	.set_rate = clk_pll_out_set_rate,
};

struct clk *tegra_clk_register_pll_out(const char *name,
		const char *parent_name, void __iomem *reg, u8 shift, u8 divider_flags)
{
	struct tegra_clk_pll_out *pll_out;
	int ret;

	pll_out = kzalloc(sizeof(*pll_out), GFP_KERNEL);
	if (!pll_out)
		return NULL;

	pll_out->div = tegra_clk_divider_alloc(NULL, NULL, reg, 0, divider_flags, shift + 8, 8, 1);
	if (!pll_out->div) {
		kfree(pll_out);
		return NULL;
	}

	pll_out->parent = parent_name;
	pll_out->hw.clk.name = name;
	pll_out->hw.clk.ops = &tegra_clk_pll_out_ops;
	pll_out->hw.clk.parent_names = (pll_out->parent ? &pll_out->parent : NULL);
	pll_out->hw.clk.num_parents = (pll_out->parent ? 1 : 0);

	pll_out->reg = reg;
	pll_out->enb_bit_idx = shift + 1;
	pll_out->rst_bit_idx = shift;

	ret = bclk_register(&pll_out->hw.clk);
	if (ret) {
		tegra_clk_divider_free(pll_out->div);
		kfree(pll_out);
		return ERR_PTR(ret);
	}

	return &pll_out->hw.clk;
}

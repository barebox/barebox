// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */

#include <common.h>
#include <clock.h>
#include <of.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>

#include "pmc.h"

#define to_clk_plldiv(_hw) container_of(_hw, struct clk_plldiv, hw)

struct clk_plldiv {
	struct clk_hw hw;
	struct regmap *regmap;
	const char *parent;
};

static unsigned long clk_plldiv_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct clk_plldiv *plldiv = to_clk_plldiv(hw);
	unsigned int mckr;

	regmap_read(plldiv->regmap, AT91_PMC_MCKR, &mckr);

	if (mckr & AT91_PMC_PLLADIV2)
		return parent_rate / 2;

	return parent_rate;
}

static long clk_plldiv_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *parent_rate)
{
	unsigned long div;

	if (rate > *parent_rate)
		return *parent_rate;
	div = *parent_rate / 2;
	if (rate < div)
		return div;

	if (rate - div < *parent_rate - rate)
		return div;

	return *parent_rate;
}

static int clk_plldiv_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct clk_plldiv *plldiv = to_clk_plldiv(hw);

	if ((parent_rate != rate) && (parent_rate / 2 != rate))
		return -EINVAL;

	regmap_write_bits(plldiv->regmap, AT91_PMC_MCKR, AT91_PMC_PLLADIV2,
			  parent_rate != rate ? AT91_PMC_PLLADIV2 : 0);

	return 0;
}

static const struct clk_ops plldiv_ops = {
	.recalc_rate = clk_plldiv_recalc_rate,
	.round_rate = clk_plldiv_round_rate,
	.set_rate = clk_plldiv_set_rate,
};

struct clk * __init
at91_clk_register_plldiv(struct regmap *regmap, const char *name,
			 const char *parent_name)
{
	int ret;
	struct clk_plldiv *plldiv;

	plldiv = xzalloc(sizeof(*plldiv));

	plldiv->hw.clk.name = name;
	plldiv->hw.clk.ops  = &plldiv_ops;

	if (parent_name) {
		plldiv->parent = parent_name;
		plldiv->hw.clk.parent_names = &plldiv->parent;
		plldiv->hw.clk.num_parents = 1;
	}

	/* init.flags = CLK_SET_RATE_GATE; */

	plldiv->regmap = regmap;

	ret = bclk_register(&plldiv->hw.clk);
	if (ret) {
		kfree(plldiv);
		return ERR_PTR(ret);
	}

	return &plldiv->hw.clk;
}

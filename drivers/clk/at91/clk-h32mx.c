// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-h32mx.c
 *
 *  Copyright (C) 2014 Atmel
 *
 * Alexandre Belloni <alexandre.belloni@free-electrons.com>
 */

#include <common.h>
#include <clock.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <linux/regmap.h>


#include "pmc.h"

#define H32MX_MAX_FREQ	90000000

struct clk_sama5d4_h32mx {
	struct clk_hw hw;
	struct regmap *regmap;
	const char *parent;
};

#define to_clk_sama5d4_h32mx(_hw) container_of(_hw, struct clk_sama5d4_h32mx, hw)

static unsigned long clk_sama5d4_h32mx_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct clk_sama5d4_h32mx *h32mxclk = to_clk_sama5d4_h32mx(hw);
	unsigned int mckr;

	regmap_read(h32mxclk->regmap, AT91_PMC_MCKR, &mckr);
	if (mckr & AT91_PMC_H32MXDIV)
		return parent_rate / 2;

	if (parent_rate > H32MX_MAX_FREQ)
		pr_warn("H32MX clock is too fast\n");
	return parent_rate;
}

static long clk_sama5d4_h32mx_round_rate(struct clk_hw *hw, unsigned long rate,
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

static int clk_sama5d4_h32mx_set_rate(struct clk_hw *hw, unsigned long rate,
				    unsigned long parent_rate)
{
	struct clk_sama5d4_h32mx *h32mxclk = to_clk_sama5d4_h32mx(hw);
	u32 mckr = 0;

	if (parent_rate != rate && (parent_rate / 2) != rate)
		return -EINVAL;

	if ((parent_rate / 2) == rate)
		mckr = AT91_PMC_H32MXDIV;

	regmap_update_bits(h32mxclk->regmap, AT91_PMC_MCKR,
			   AT91_PMC_H32MXDIV, mckr);

	return 0;
}

static const struct clk_ops h32mx_ops = {
	.recalc_rate = clk_sama5d4_h32mx_recalc_rate,
	.round_rate = clk_sama5d4_h32mx_round_rate,
	.set_rate = clk_sama5d4_h32mx_set_rate,
};

struct clk * __init
at91_clk_register_h32mx(struct regmap *regmap, const char *name,
			const char *parent_name)
{
	struct clk_sama5d4_h32mx *h32mxclk;
	int ret;

	h32mxclk = kzalloc(sizeof(*h32mxclk), GFP_KERNEL);
	if (!h32mxclk)
		return ERR_PTR(-ENOMEM);

	h32mxclk->parent = parent_name;
	h32mxclk->hw.clk.name = name;
	h32mxclk->hw.clk.ops = &h32mx_ops;
	h32mxclk->hw.clk.parent_names = &h32mxclk->parent;
	h32mxclk->hw.clk.num_parents = 1;
	/* h32mxclk.hw.flags = CLK_SET_RATE_GATE; */

	h32mxclk->regmap = regmap;

	ret = bclk_register(&h32mxclk->hw.clk);
	if (ret) {
		kfree(h32mxclk);
		return ERR_PTR(ret);
	}

	return &h32mxclk->hw.clk;
}

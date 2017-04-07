/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <common.h>
#include <clock.h>
#include <of.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <regmap.h>

#include "pmc.h"

#define to_clk_plldiv(hw) container_of(clk, struct clk_plldiv, clk)

struct clk_plldiv {
	struct clk clk;
	struct regmap *regmap;
	const char *parent;
};

static unsigned long clk_plldiv_recalc_rate(struct clk *clk,
					    unsigned long parent_rate)
{
	struct clk_plldiv *plldiv = to_clk_plldiv(clk);
	unsigned int mckr;

	regmap_read(plldiv->regmap, AT91_PMC_MCKR, &mckr);

	if (mckr & AT91_PMC_PLLADIV2)
		return parent_rate / 2;

	return parent_rate;
}

static long clk_plldiv_round_rate(struct clk *clk, unsigned long rate,
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

static int clk_plldiv_set_rate(struct clk *clk, unsigned long rate,
			       unsigned long parent_rate)
{
	struct clk_plldiv *plldiv = to_clk_plldiv(clk);

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

static struct clk *
at91_clk_register_plldiv(struct regmap *regmap, const char *name,
			 const char *parent_name)
{
	int ret;
	struct clk_plldiv *plldiv;

	plldiv = xzalloc(sizeof(*plldiv));

	plldiv->clk.name = name;
	plldiv->clk.ops  = &plldiv_ops;

	if (parent_name) {
		plldiv->parent = parent_name;
		plldiv->clk.parent_names = &plldiv->parent;
		plldiv->clk.num_parents = 1;
	}

	/* init.flags = CLK_SET_RATE_GATE; */

	plldiv->regmap = regmap;

	ret = clk_register(&plldiv->clk);
	if (ret) {
		kfree(plldiv);
		return ERR_PTR(ret);
	}

	return &plldiv->clk;
}

static int
of_at91sam9x5_clk_plldiv_setup(struct device_node *np)
{
	struct clk *clk;
	const char *parent_name;
	const char *name = np->name;
	struct regmap *regmap;

	parent_name = of_clk_get_parent_name(np, 0);

	of_property_read_string(np, "clock-output-names", &name);

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	clk = at91_clk_register_plldiv(regmap, name, parent_name);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(at91sam9x5_clk_plldiv, "atmel,at91sam9x5-clk-plldiv",
	       of_at91sam9x5_clk_plldiv_setup);

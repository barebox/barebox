// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/clk/at91/clk-slow.c
 *
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */

#include <common.h>
#include <clock.h>
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <linux/overflow.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>

#include "pmc.h"

struct clk_sam9260_slow {
	struct clk_hw hw;
	struct regmap *regmap;
	const char *parent_names[];
};

#define to_clk_sam9260_slow(_hw) container_of(_hw, struct clk_sam9260_slow, hw)

static int clk_sam9260_slow_get_parent(struct clk_hw *hw)
{
	struct clk_sam9260_slow *slowck = to_clk_sam9260_slow(hw);
	unsigned int status;

	regmap_read(slowck->regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_OSCSEL ? 1 : 0;
}

static const struct clk_ops sam9260_slow_ops = {
	.get_parent = clk_sam9260_slow_get_parent,
};

struct clk * __init
at91_clk_register_sam9260_slow(struct regmap *regmap,
			       const char *name,
			       const char **parent_names,
			       int num_parents)
{
	struct clk_sam9260_slow *slowck;
	int ret;

	if (!name)
		return ERR_PTR(-EINVAL);

	if (!parent_names || !num_parents)
		return ERR_PTR(-EINVAL);

	slowck = xzalloc(struct_size(slowck, parent_names, num_parents));
	slowck->hw.clk.name = name;
	slowck->hw.clk.ops = &sam9260_slow_ops;
	memcpy(slowck->parent_names, parent_names,
	       num_parents * sizeof(slowck->parent_names[0]));
	slowck->hw.clk.parent_names = slowck->parent_names;
	slowck->hw.clk.num_parents = num_parents;
	slowck->regmap = regmap;

	ret = bclk_register(&slowck->hw.clk);
	if (ret) {
		kfree(slowck);
		return ERR_PTR(ret);
	}

	return &slowck->hw.clk;
}

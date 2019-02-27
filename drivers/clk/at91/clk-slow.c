/*
 * drivers/clk/at91/clk-slow.c
 *
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
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <regmap.h>

#include "pmc.h"

struct clk_sam9260_slow {
	struct clk clk;
	struct regmap *regmap;
	const char *parent_names[2];
};

#define to_clk_sam9260_slow(clk) container_of(clk, struct clk_sam9260_slow, clk)

static int clk_sam9260_slow_get_parent(struct clk *clk)
{
	struct clk_sam9260_slow *slowck = to_clk_sam9260_slow(clk);
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

	slowck = xzalloc(sizeof(*slowck));
	slowck->clk.name = name;
	slowck->clk.ops = &sam9260_slow_ops;
	memcpy(slowck->parent_names, parent_names,
	       num_parents * sizeof(slowck->parent_names[0]));
	slowck->clk.parent_names = slowck->parent_names;
	slowck->clk.num_parents = num_parents;
	slowck->regmap = regmap;

	ret = clk_register(&slowck->clk);
	if (ret) {
		kfree(slowck);
		return ERR_PTR(ret);
	}

	return &slowck->clk;
}

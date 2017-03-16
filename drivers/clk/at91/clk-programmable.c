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
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <regmap.h>

#include "pmc.h"

#define PROG_SOURCE_MAX		5
#define PROG_ID_MAX		7

#define PROG_STATUS_MASK(id)	(1 << ((id) + 8))
#define PROG_PRES_MASK		0x7
#define PROG_PRES(layout, pckr)	((pckr >> layout->pres_shift) & PROG_PRES_MASK)
#define PROG_MAX_RM9200_CSS	3

struct clk_programmable_layout {
	u8 pres_shift;
	u8 css_mask;
	u8 have_slck_mck;
};

struct clk_programmable {
	struct clk clk;
	struct regmap *regmap;
	u8 id;
	const struct clk_programmable_layout *layout;
	const char *parent_names[PROG_SOURCE_MAX];
};

#define to_clk_programmable(clk) container_of(clk, struct clk_programmable, clk)

static unsigned long clk_programmable_recalc_rate(struct clk *clk,
						  unsigned long parent_rate)
{
	struct clk_programmable *prog = to_clk_programmable(clk);
	unsigned int pckr;

	regmap_read(prog->regmap, AT91_PMC_PCKR(prog->id), &pckr);

	return parent_rate >> PROG_PRES(prog->layout, pckr);
}

static int clk_programmable_set_parent(struct clk *clk, u8 index)
{
	struct clk_programmable *prog = to_clk_programmable(clk);
	const struct clk_programmable_layout *layout = prog->layout;
	unsigned int mask = layout->css_mask;
	unsigned int pckr = index;

	if (layout->have_slck_mck)
		mask |= AT91_PMC_CSSMCK_MCK;

	if (index > layout->css_mask) {
		if (index > PROG_MAX_RM9200_CSS && !layout->have_slck_mck)
			return -EINVAL;

		pckr |= AT91_PMC_CSSMCK_MCK;
	}

	regmap_write_bits(prog->regmap, AT91_PMC_PCKR(prog->id), mask, pckr);

	return 0;
}

static int clk_programmable_get_parent(struct clk *clk)
{
	struct clk_programmable *prog = to_clk_programmable(clk);
	const struct clk_programmable_layout *layout = prog->layout;
	unsigned int pckr;
	u8 ret;

	regmap_read(prog->regmap, AT91_PMC_PCKR(prog->id), &pckr);

	ret = pckr & layout->css_mask;

	if (layout->have_slck_mck && (pckr & AT91_PMC_CSSMCK_MCK) && !ret)
		ret = PROG_MAX_RM9200_CSS + 1;

	return ret;
}

static int clk_programmable_set_rate(struct clk *clk, unsigned long rate,
				     unsigned long parent_rate)
{
	struct clk_programmable *prog = to_clk_programmable(clk);
	const struct clk_programmable_layout *layout = prog->layout;
	unsigned long div = parent_rate / rate;
	unsigned int pckr;
	int shift = 0;

	regmap_read(prog->regmap, AT91_PMC_PCKR(prog->id), &pckr);

	if (!div)
		return -EINVAL;

	shift = fls(div) - 1;

	if (div != (1 << shift))
		return -EINVAL;

	if (shift >= PROG_PRES_MASK)
		return -EINVAL;

	regmap_write_bits(prog->regmap, AT91_PMC_PCKR(prog->id),
			   PROG_PRES_MASK << layout->pres_shift,
			   shift << layout->pres_shift);

	return 0;
}

static const struct clk_ops programmable_ops = {
	.recalc_rate = clk_programmable_recalc_rate,
	.get_parent = clk_programmable_get_parent,
	.set_parent = clk_programmable_set_parent,
	.set_rate = clk_programmable_set_rate,
};

static struct clk *
at91_clk_register_programmable(struct regmap *regmap,
			       const char *name, const char **parent_names,
			       u8 num_parents, u8 id,
			       const struct clk_programmable_layout *layout)
{
	struct clk_programmable *prog;
	int ret;

	if (id > PROG_ID_MAX)
		return ERR_PTR(-EINVAL);

	prog = kzalloc(sizeof(*prog), GFP_KERNEL);
	if (!prog)
		return ERR_PTR(-ENOMEM);

	prog->clk.name = name;
	prog->clk.ops = &programmable_ops;
	memcpy(prog->parent_names, parent_names,
	       num_parents * sizeof(prog->parent_names[0]));
	prog->clk.parent_names = &prog->parent_names[0];
	prog->clk.num_parents = num_parents;
	/* init.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE; */

	prog->id = id;
	prog->layout = layout;
	prog->regmap = regmap;

	ret = clk_register(&prog->clk);
	if (ret) {
		kfree(prog);
		return ERR_PTR(ret);
	}

	return &prog->clk;
}

static const struct clk_programmable_layout at91rm9200_programmable_layout = {
	.pres_shift = 2,
	.css_mask = 0x3,
	.have_slck_mck = 0,
};

static const struct clk_programmable_layout at91sam9g45_programmable_layout = {
	.pres_shift = 2,
	.css_mask = 0x3,
	.have_slck_mck = 1,
};

static const struct clk_programmable_layout at91sam9x5_programmable_layout = {
	.pres_shift = 4,
	.css_mask = 0x7,
	.have_slck_mck = 0,
};

static int
of_at91_clk_prog_setup(struct device_node *np,
		       const struct clk_programmable_layout *layout)
{
	int num;
	u32 id;
	struct clk *clk;
	unsigned int num_parents;
	const char *parent_names[PROG_SOURCE_MAX];
	const char *name;
	struct device_node *progclknp;
	struct regmap *regmap;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents == 0 || num_parents > PROG_SOURCE_MAX)
		return -EINVAL;

	of_clk_parent_fill(np, parent_names, num_parents);

	num = of_get_child_count(np);
	if (!num || num > (PROG_ID_MAX + 1))
		return -EINVAL;

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	for_each_child_of_node(np, progclknp) {
		if (of_property_read_u32(progclknp, "reg", &id))
			continue;

		if (of_property_read_string(np, "clock-output-names", &name))
			name = progclknp->name;

		clk = at91_clk_register_programmable(regmap, name,
						     parent_names, num_parents,
						     id, layout);
		if (IS_ERR(clk))
			continue;

		of_clk_add_provider(progclknp, of_clk_src_simple_get, clk);
	}

	return 0;
}


static void __init of_at91rm9200_clk_prog_setup(struct device_node *np)
{
	of_at91_clk_prog_setup(np, &at91rm9200_programmable_layout);
}
CLK_OF_DECLARE(at91rm9200_clk_prog, "atmel,at91rm9200-clk-programmable",
	       of_at91rm9200_clk_prog_setup);

static int of_at91sam9g45_clk_prog_setup(struct device_node *np)
{
	return of_at91_clk_prog_setup(np, &at91sam9g45_programmable_layout);
}
CLK_OF_DECLARE(at91sam9g45_clk_prog, "atmel,at91sam9g45-clk-programmable",
	       of_at91sam9g45_clk_prog_setup);

static int of_at91sam9x5_clk_prog_setup(struct device_node *np)
{
	return of_at91_clk_prog_setup(np, &at91sam9x5_programmable_layout);
}
CLK_OF_DECLARE(at91sam9x5_clk_prog, "atmel,at91sam9x5-clk-programmable",
	       of_at91sam9x5_clk_prog_setup);

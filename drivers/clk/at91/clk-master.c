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

#define MASTER_SOURCE_MAX	4

#define MASTER_PRES_MASK	0x7
#define MASTER_PRES_MAX		MASTER_PRES_MASK
#define MASTER_DIV_SHIFT	8
#define MASTER_DIV_MASK		0x3

struct clk_master_characteristics {
	struct clk_range output;
	u32 divisors[4];
	u8 have_div3_pres;
};

struct clk_master_layout {
	u32 mask;
	u8 pres_shift;
};

#define to_clk_master(clk) container_of(clk, struct clk_master, clk)

struct clk_master {
	struct clk clk;
	struct regmap *regmap;
	const struct clk_master_layout *layout;
	const struct clk_master_characteristics *characteristics;
	const char *parents[MASTER_SOURCE_MAX];
};

static inline bool clk_master_ready(struct regmap *regmap)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_MCKRDY ? 1 : 0;
}

static int clk_master_enable(struct clk *clk)
{
	struct clk_master *master = to_clk_master(clk);

	while (!clk_master_ready(master->regmap))
		barrier();

	return 0;
}

static int clk_master_is_enabled(struct clk *clk)
{
	struct clk_master *master = to_clk_master(clk);

	return clk_master_ready(master->regmap);
}

static unsigned long clk_master_recalc_rate(struct clk *clk,
					    unsigned long parent_rate)
{
	u8 pres;
	u8 div;
	unsigned long rate = parent_rate;
	struct clk_master *master = to_clk_master(clk);
	const struct clk_master_layout *layout = master->layout;
	const struct clk_master_characteristics *characteristics =
						master->characteristics;
	unsigned int mckr;

	regmap_read(master->regmap, AT91_PMC_MCKR, &mckr);
	mckr &= layout->mask;

	pres = (mckr >> layout->pres_shift) & MASTER_PRES_MASK;
	div = (mckr >> MASTER_DIV_SHIFT) & MASTER_DIV_MASK;

	if (characteristics->have_div3_pres && pres == MASTER_PRES_MAX)
		rate /= 3;
	else
		rate >>= pres;

	rate /= characteristics->divisors[div];

	if (rate < characteristics->output.min)
		pr_warn("master clk is underclocked");
	else if (rate > characteristics->output.max)
		pr_warn("master clk is overclocked");

	return rate;
}

static int clk_master_get_parent(struct clk *clk)
{
	struct clk_master *master = to_clk_master(clk);
	unsigned int mckr;

	regmap_read(master->regmap, AT91_PMC_MCKR, &mckr);

	return mckr & AT91_PMC_CSS;
}

static const struct clk_ops master_ops = {
	.enable = clk_master_enable,
	.is_enabled = clk_master_is_enabled,
	.recalc_rate = clk_master_recalc_rate,
	.get_parent = clk_master_get_parent,
};

static struct clk *
at91_clk_register_master(struct regmap *regmap,
			 const char *name, int num_parents,
			 const char **parent_names,
			 const struct clk_master_layout *layout,
			 const struct clk_master_characteristics *characteristics)
{
	int ret;
	const size_t parent_names_size = num_parents * sizeof(parent_names[0]);
	struct clk_master *master;

	if (!name || !num_parents || !parent_names)
		return ERR_PTR(-EINVAL);

	master = xzalloc(sizeof(*master));

	master->clk.name = name;
	master->clk.ops = &master_ops;
	memcpy(master->parents, parent_names, parent_names_size);
	master->clk.parent_names = master->parents;
	master->clk.num_parents = num_parents;

	master->layout = layout;
	master->characteristics = characteristics;
	master->regmap = regmap;

	ret = clk_register(&master->clk);
	if (ret) {
		kfree(master);
		return ERR_PTR(ret);
	}

	return &master->clk;
}


static const struct clk_master_layout at91rm9200_master_layout = {
	.mask = 0x31F,
	.pres_shift = 2,
};

static const struct clk_master_layout at91sam9x5_master_layout = {
	.mask = 0x373,
	.pres_shift = 4,
};


static struct clk_master_characteristics *
of_at91_clk_master_get_characteristics(struct device_node *np)
{
	struct clk_master_characteristics *characteristics;

	characteristics = xzalloc(sizeof(*characteristics));

	if (of_at91_get_clk_range(np, "atmel,clk-output-range", &characteristics->output))
		goto out_free_characteristics;

	of_property_read_u32_array(np, "atmel,clk-divisors",
				   characteristics->divisors, 4);

	characteristics->have_div3_pres =
		of_property_read_bool(np, "atmel,master-clk-have-div3-pres");

	return characteristics;

out_free_characteristics:
	kfree(characteristics);
	return NULL;
}

static int
of_at91_clk_master_setup(struct device_node *np,
			 const struct clk_master_layout *layout)
{
	struct clk *clk;
	unsigned int num_parents;
	const char *parent_names[MASTER_SOURCE_MAX];
	const char *name = np->name;
	struct clk_master_characteristics *characteristics;
	struct regmap *regmap;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents == 0 || num_parents > MASTER_SOURCE_MAX)
		return -EINVAL;

	of_clk_parent_fill(np, parent_names, num_parents);

	of_property_read_string(np, "clock-output-names", &name);

	characteristics = of_at91_clk_master_get_characteristics(np);
	if (!characteristics)
		return -EINVAL;

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	clk = at91_clk_register_master(regmap, name, num_parents,
				       parent_names, layout,
				       characteristics);
	if (IS_ERR(clk)) {
		kfree(characteristics);
		return PTR_ERR(clk);
	}

	return of_clk_add_provider(np, of_clk_src_simple_get, clk);
}

static void __init of_at91rm9200_clk_master_setup(struct device_node *np)
{
	of_at91_clk_master_setup(np, &at91rm9200_master_layout);
}
CLK_OF_DECLARE(at91rm9200_clk_master, "atmel,at91rm9200-clk-master",
	       of_at91rm9200_clk_master_setup);

static void __init of_at91sam9x5_clk_master_setup(struct device_node *np)
{
	of_at91_clk_master_setup(np, &at91sam9x5_master_layout);
}
CLK_OF_DECLARE(at91sam9x5_clk_master, "atmel,at91sam9x5-clk-master",
	       of_at91sam9x5_clk_master_setup);

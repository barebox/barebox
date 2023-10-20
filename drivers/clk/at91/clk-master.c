// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */
#include <common.h>
#include <clock.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <linux/overflow.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>

#include "pmc.h"

#define MASTER_PRES_MASK	0x7
#define MASTER_PRES_MAX		MASTER_PRES_MASK
#define MASTER_DIV_SHIFT	8
#define MASTER_DIV_MASK		0x3

#define to_clk_master(_hw) container_of(_hw, struct clk_master, hw)

struct clk_master {
	struct clk_hw hw;
	struct regmap *regmap;
	const struct clk_master_layout *layout;
	const struct clk_master_characteristics *characteristics;
	u32 mckr;
	const char *parents[];
};

static inline bool clk_master_ready(struct regmap *regmap)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_MCKRDY ? 1 : 0;
}

static int clk_master_enable(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);

	while (!clk_master_ready(master->regmap))
		barrier();

	return 0;
}

static int clk_master_is_enabled(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);

	return clk_master_ready(master->regmap);
}

static unsigned long clk_master_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	u8 pres;
	u8 div;
	unsigned long rate = parent_rate;
	struct clk_master *master = to_clk_master(hw);
	const struct clk_master_layout *layout = master->layout;
	const struct clk_master_characteristics *characteristics =
						master->characteristics;
	unsigned int mckr;

	regmap_read(master->regmap, master->layout->offset, &mckr);
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

static int clk_master_get_parent(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned int mckr;

	regmap_read(master->regmap, master->layout->offset, &mckr);

	return mckr & AT91_PMC_CSS;
}

static const struct clk_ops master_ops = {
	.enable = clk_master_enable,
	.is_enabled = clk_master_is_enabled,
	.recalc_rate = clk_master_recalc_rate,
	.get_parent = clk_master_get_parent,
};

struct clk * __init
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

	master = xzalloc(struct_size(master, parents, num_parents));

	master->hw.clk.name = name;
	master->hw.clk.ops = &master_ops;
	memcpy(master->parents, parent_names, parent_names_size);
	master->hw.clk.parent_names = master->parents;
	master->hw.clk.num_parents = num_parents;

	master->layout = layout;
	master->characteristics = characteristics;
	master->regmap = regmap;

	ret = bclk_register(&master->hw.clk);
	if (ret) {
		kfree(master);
		return ERR_PTR(ret);
	}

	return &master->hw.clk;
}


const struct clk_master_layout at91rm9200_master_layout = {
	.mask = 0x31F,
	.pres_shift = 2,
	.offset = AT91_PMC_MCKR,
};

const struct clk_master_layout at91sam9x5_master_layout = {
	.mask = 0x373,
	.pres_shift = 4,
	.offset = AT91_PMC_MCKR,
};

/*
 *  Copyright (C) 2015 Atmel Corporation,
 *                     Nicolas Ferre <nicolas.ferre@atmel.com>
 *
 * Based on clk-programmable & clk-peripheral drivers by Boris BREZILLON.
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

#define GENERATED_MAX_DIV	255

struct clk_generated {
	struct clk hw;
	struct regmap *regmap;
	struct clk_range range;
	u32 id;
	u32 gckdiv;
	u8 parent_id;
};

#define to_clk_generated(hw) \
	container_of(hw, struct clk_generated, hw)

static int clk_generated_enable(struct clk *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);

	pr_debug("GCLK: %s, gckdiv = %d, parent id = %d\n",
		 __func__, gck->gckdiv, gck->parent_id);

	regmap_write(gck->regmap, AT91_PMC_PCR,
		     (gck->id & AT91_PMC_PCR_PID_MASK));
	regmap_update_bits(gck->regmap, AT91_PMC_PCR,
			   AT91_PMC_PCR_GCKDIV_MASK | AT91_PMC_PCR_GCKCSS_MASK |
			   AT91_PMC_PCR_CMD | AT91_PMC_PCR_GCKEN,
			   AT91_PMC_PCR_GCKCSS(gck->parent_id) |
			   AT91_PMC_PCR_CMD |
			   AT91_PMC_PCR_GCKDIV(gck->gckdiv) |
			   AT91_PMC_PCR_GCKEN);
	return 0;
}

static void clk_generated_disable(struct clk *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);

	regmap_write(gck->regmap, AT91_PMC_PCR,
		     (gck->id & AT91_PMC_PCR_PID_MASK));
	regmap_update_bits(gck->regmap, AT91_PMC_PCR,
			   AT91_PMC_PCR_CMD | AT91_PMC_PCR_GCKEN,
			   AT91_PMC_PCR_CMD);
}

static int clk_generated_is_enabled(struct clk *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);
	unsigned int status;

	regmap_write(gck->regmap, AT91_PMC_PCR,
		     (gck->id & AT91_PMC_PCR_PID_MASK));
	regmap_read(gck->regmap, AT91_PMC_PCR, &status);

	return status & AT91_PMC_PCR_GCKEN ? 1 : 0;
}

static unsigned long
clk_generated_recalc_rate(struct clk *hw,
			  unsigned long parent_rate)
{
	struct clk_generated *gck = to_clk_generated(hw);

	return DIV_ROUND_CLOSEST(parent_rate, gck->gckdiv + 1);
}

/* No modification of hardware as we have the flag CLK_SET_PARENT_GATE set */
static int clk_generated_set_parent(struct clk *hw, u8 index)
{
	struct clk_generated *gck = to_clk_generated(hw);

	if (index >= clk_get_num_parents(hw))
		return -EINVAL;

	gck->parent_id = index;
	return 0;
}

static int clk_generated_get_parent(struct clk *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);

	return gck->parent_id;
}

/* No modification of hardware as we have the flag CLK_SET_RATE_GATE set */
static int clk_generated_set_rate(struct clk *hw,
				  unsigned long rate,
				  unsigned long parent_rate)
{
	struct clk_generated *gck = to_clk_generated(hw);
	u32 div;

	if (!rate)
		return -EINVAL;

	if (gck->range.max && rate > gck->range.max)
		return -EINVAL;

	div = DIV_ROUND_CLOSEST(parent_rate, rate);
	if (div > GENERATED_MAX_DIV + 1 || !div)
		return -EINVAL;

	gck->gckdiv = div - 1;
	return 0;
}

static const struct clk_ops generated_ops = {
	.enable = clk_generated_enable,
	.disable = clk_generated_disable,
	.is_enabled = clk_generated_is_enabled,
	.recalc_rate = clk_generated_recalc_rate,
	.get_parent = clk_generated_get_parent,
	.set_parent = clk_generated_set_parent,
	.set_rate = clk_generated_set_rate,
};

/**
 * clk_generated_startup - Initialize a given clock to its default parent and
 * divisor parameter.
 *
 * @gck:	Generated clock to set the startup parameters for.
 *
 * Take parameters from the hardware and update local clock configuration
 * accordingly.
 */
static void clk_generated_startup(struct clk_generated *gck)
{
	u32 tmp;

	regmap_write(gck->regmap, AT91_PMC_PCR,
		     (gck->id & AT91_PMC_PCR_PID_MASK));
	regmap_read(gck->regmap, AT91_PMC_PCR, &tmp);

	gck->parent_id = (tmp & AT91_PMC_PCR_GCKCSS_MASK)
					>> AT91_PMC_PCR_GCKCSS_OFFSET;
	gck->gckdiv = (tmp & AT91_PMC_PCR_GCKDIV_MASK)
					>> AT91_PMC_PCR_GCKDIV_OFFSET;
}

struct clk * __init
at91_clk_register_generated(struct regmap *regmap,
			    const char *name, const char **parent_names,
			    u8 num_parents, u8 id, bool pll_audio,
			    const struct clk_range *range)
{
	size_t parents_array_size;
	struct clk_generated *gck;
	struct clk *hw;
	int ret;

	gck = kzalloc(sizeof(*gck), GFP_KERNEL);
	if (!gck)
		return ERR_PTR(-ENOMEM);

	gck->id = id;
	gck->hw.name = name;
	gck->hw.ops = &generated_ops;

	parents_array_size = num_parents * sizeof(gck->hw.parent_names[0]);
	gck->hw.parent_names = xmemdup(parent_names, parents_array_size);
	gck->hw.num_parents = num_parents;

	/* gck->hw.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE; */
	gck->regmap = regmap;
	gck->range = *range;
	/* gck->audio_pll_allowed = pll_audio; */

	hw = &gck->hw;
	ret = clk_register(&gck->hw);
	if (ret) {
		kfree(gck);
		hw = ERR_PTR(ret);
	} else
		clk_generated_startup(gck);

	return hw;
}

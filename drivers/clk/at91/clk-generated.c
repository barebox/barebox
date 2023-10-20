// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2015 Atmel Corporation,
 *                     Nicolas Ferre <nicolas.ferre@atmel.com>
 *
 * Based on clk-programmable & clk-peripheral drivers by Boris BREZILLON.
 */

#include <common.h>
#include <clock.h>
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/bitfield.h>

#include "pmc.h"

#define GENERATED_MAX_DIV	255

#define GCK_INDEX_DT_AUDIO_PLL	5

struct clk_generated {
	struct clk_hw hw;
	struct regmap *regmap;
	struct clk_range range;
	u32 id;
	u32 gckdiv;
	const struct clk_pcr_layout *layout;
	u8 parent_id;
	bool audio_pll_allowed;
};

#define to_clk_generated(_hw) \
	container_of(_hw, struct clk_generated, hw)

static int clk_generated_enable(struct clk_hw *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);

	pr_debug("GCLK: %s, gckdiv = %d, parent id = %d\n",
		 __func__, gck->gckdiv, gck->parent_id);

	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_update_bits(gck->regmap, gck->layout->offset,
			   AT91_PMC_PCR_GCKDIV_MASK | gck->layout->gckcss_mask |
			   gck->layout->cmd | AT91_PMC_PCR_GCKEN,
			   field_prep(gck->layout->gckcss_mask, gck->parent_id) |
			   gck->layout->cmd |
			   FIELD_PREP(AT91_PMC_PCR_GCKDIV_MASK, gck->gckdiv) |
			   AT91_PMC_PCR_GCKEN);
	return 0;
}

static void clk_generated_disable(struct clk_hw *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);

	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_update_bits(gck->regmap, gck->layout->offset,
			   gck->layout->cmd | AT91_PMC_PCR_GCKEN,
			   gck->layout->cmd);
}

static int clk_generated_is_enabled(struct clk_hw *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);
	unsigned int status;

	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_read(gck->regmap, gck->layout->offset, &status);

	return status & AT91_PMC_PCR_GCKEN ? 1 : 0;
}

static unsigned long
clk_generated_recalc_rate(struct clk_hw *hw,
			  unsigned long parent_rate)
{
	struct clk_generated *gck = to_clk_generated(hw);

	return DIV_ROUND_CLOSEST(parent_rate, gck->gckdiv + 1);
}

/* No modification of hardware as we have the flag CLK_SET_PARENT_GATE set */
static int clk_generated_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_generated *gck = to_clk_generated(hw);

	if (index >= clk_hw_get_num_parents(hw))
		return -EINVAL;

	gck->parent_id = index;
	return 0;
}

static int clk_generated_get_parent(struct clk_hw *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);

	return gck->parent_id;
}

/* No modification of hardware as we have the flag CLK_SET_RATE_GATE set */
static int clk_generated_set_rate(struct clk_hw *hw,
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

	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_read(gck->regmap, gck->layout->offset, &tmp);

	gck->parent_id = field_get(gck->layout->gckcss_mask, tmp);
	gck->gckdiv = FIELD_GET(AT91_PMC_PCR_GCKDIV_MASK, tmp);
}

struct clk * __init
at91_clk_register_generated(struct regmap *regmap,
			    const struct clk_pcr_layout *layout,
			    const char *name, const char **parent_names,
			    u8 num_parents, u8 id, bool pll_audio,
			    const struct clk_range *range)
{
	size_t parents_array_size;
	struct clk_generated *gck;
	struct clk *clk;
	int ret;

	gck = kzalloc(sizeof(*gck), GFP_KERNEL);
	if (!gck)
		return ERR_PTR(-ENOMEM);

	gck->id = id;
	gck->hw.clk.name = name;
	gck->hw.clk.ops = &generated_ops;

	parents_array_size = num_parents * sizeof(gck->hw.clk.parent_names[0]);
	gck->hw.clk.parent_names = xmemdup(parent_names, parents_array_size);
	gck->hw.clk.num_parents = num_parents;

	/* gck->hw.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE | CLK_SET_PARENT; */
	gck->regmap = regmap;
	gck->range = *range;
	gck->audio_pll_allowed = pll_audio;
	gck->layout = layout;

	clk_generated_startup(gck);
	clk = &gck->hw.clk;
	ret = bclk_register(&gck->hw.clk);
	if (ret) {
		kfree(gck);
		clk = ERR_PTR(ret);
	} else {
		pmc_register_id(id);
	}

	return clk;
}

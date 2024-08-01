// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2015 Atmel Corporation,
 *                     Nicolas Ferre <nicolas.ferre@atmel.com>
 *
 * Based on clk-programmable & clk-peripheral drivers by Boris BREZILLON.
 */

#include <linux/bitfield.h>
#include <linux/printk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk/at91_pmc.h>
#include <of.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/kernel.h>

#include "pmc.h"

#define GENERATED_MAX_DIV	255

struct clk_generated {
	struct clk_hw hw;
	struct regmap *regmap;
	struct clk_range range;
	spinlock_t *lock;
	u32 *mux_table;
	u32 id;
	u32 gckdiv;
	const struct clk_pcr_layout *layout;
	struct at91_clk_pms pms;
	u8 parent_id;
	int chg_pid;
};

#define to_clk_generated(hw) \
	container_of(hw, struct clk_generated, hw)

static int clk_generated_set(struct clk_generated *gck, int status)
{
	unsigned long flags;
	unsigned int enable = status ? AT91_PMC_PCR_GCKEN : 0;

	spin_lock_irqsave(gck->lock, flags);
	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_update_bits(gck->regmap, gck->layout->offset,
			   AT91_PMC_PCR_GCKDIV_MASK | gck->layout->gckcss_mask |
			   gck->layout->cmd | enable,
			   field_prep(gck->layout->gckcss_mask, gck->parent_id) |
			   gck->layout->cmd |
			   FIELD_PREP(AT91_PMC_PCR_GCKDIV_MASK, gck->gckdiv) |
			   enable);
	spin_unlock_irqrestore(gck->lock, flags);

	return 0;
}

static int clk_generated_enable(struct clk_hw *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);

	pr_debug("GCLK: %s, gckdiv = %d, parent id = %d\n",
		 __func__, gck->gckdiv, gck->parent_id);

	clk_generated_set(gck, 1);

	return 0;
}

static void clk_generated_disable(struct clk_hw *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);
	unsigned long flags;

	spin_lock_irqsave(gck->lock, flags);
	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_update_bits(gck->regmap, gck->layout->offset,
			   gck->layout->cmd | AT91_PMC_PCR_GCKEN,
			   gck->layout->cmd);
	spin_unlock_irqrestore(gck->lock, flags);
}

static int clk_generated_is_enabled(struct clk_hw *hw)
{
	struct clk_generated *gck = to_clk_generated(hw);
	unsigned long flags;
	unsigned int status;

	spin_lock_irqsave(gck->lock, flags);
	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_read(gck->regmap, gck->layout->offset, &status);
	spin_unlock_irqrestore(gck->lock, flags);

	return !!(status & AT91_PMC_PCR_GCKEN);
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

	if (gck->mux_table)
		gck->parent_id = clk_mux_index_to_val(gck->mux_table, 0, index);
	else
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
	unsigned long flags;

	spin_lock_irqsave(gck->lock, flags);
	regmap_write(gck->regmap, gck->layout->offset,
		     (gck->id & gck->layout->pid_mask));
	regmap_read(gck->regmap, gck->layout->offset, &tmp);
	spin_unlock_irqrestore(gck->lock, flags);

	gck->parent_id = field_get(gck->layout->gckcss_mask, tmp);
	gck->gckdiv = FIELD_GET(AT91_PMC_PCR_GCKDIV_MASK, tmp);
}

struct clk_hw * __init
at91_clk_register_generated(struct regmap *regmap, spinlock_t *lock,
			    const struct clk_pcr_layout *layout,
			    const char *name, const char **parent_names,
			    u32 *mux_table, u8 num_parents, u8 id,
			    const struct clk_range *range,
			    int chg_pid)
{
	struct clk_generated *gck;
	struct clk_init_data init;
	struct clk_hw *hw;
	int ret;

	gck = kzalloc(sizeof(*gck), GFP_KERNEL);
	if (!gck)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = &generated_ops;
	init.parent_names = parent_names;
	init.num_parents = num_parents;
	init.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE;
	if (chg_pid >= 0)
		init.flags |= CLK_SET_RATE_PARENT;

	gck->id = id;
	gck->hw.init = &init;
	gck->regmap = regmap;
	gck->lock = lock;
	gck->range = *range;
	gck->chg_pid = chg_pid;
	gck->layout = layout;
	gck->mux_table = mux_table;

	clk_generated_startup(gck);
	hw = &gck->hw;
	ret = clk_hw_register(NULL, &gck->hw);
	if (ret) {
		kfree(gck);
		hw = ERR_PTR(ret);
	}

	return hw;
}

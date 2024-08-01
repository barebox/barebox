// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */

#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <of.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/printk.h>
#include <linux/kernel.h>

#include "pmc.h"

#define MASTER_PRES_MASK	0x7
#define MASTER_PRES_MAX		MASTER_PRES_MASK
#define MASTER_DIV_SHIFT	8
#define MASTER_DIV_MASK		0x7

#define PMC_MCR_CSS_SHIFT	(16)

#define MASTER_MAX_ID		4

#define to_clk_master(hw) container_of(hw, struct clk_master, hw)

struct clk_master {
	struct clk_hw hw;
	struct regmap *regmap;
	spinlock_t *lock;
	const struct clk_master_layout *layout;
	const struct clk_master_characteristics *characteristics;
	struct at91_clk_pms pms;
	u32 *mux_table;
	u32 mckr;
	int chg_pid;
	u8 id;
	u8 parent;
	u8 div;
};

static inline bool clk_master_ready(struct clk_master *master)
{
	unsigned int bit = master->id ? AT91_PMC_MCKXRDY : AT91_PMC_MCKRDY;
	unsigned int status;

	regmap_read(master->regmap, AT91_PMC_SR, &status);

	return !!(status & bit);
}

static int clk_master_prepare(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long flags;

	spin_lock_irqsave(master->lock, flags);

	while (!clk_master_ready(master))
		cpu_relax();

	spin_unlock_irqrestore(master->lock, flags);

	return 0;
}

static int clk_master_is_prepared(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long flags;
	bool status;

	spin_lock_irqsave(master->lock, flags);
	status = clk_master_ready(master);
	spin_unlock_irqrestore(master->lock, flags);

	return status;
}

static unsigned long clk_master_div_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	u8 div;
	unsigned long flags, rate = parent_rate;
	struct clk_master *master = to_clk_master(hw);
	const struct clk_master_layout *layout = master->layout;
	const struct clk_master_characteristics *characteristics =
						master->characteristics;
	unsigned int mckr;

	spin_lock_irqsave(master->lock, flags);
	regmap_read(master->regmap, master->layout->offset, &mckr);
	spin_unlock_irqrestore(master->lock, flags);

	mckr &= layout->mask;

	div = (mckr >> MASTER_DIV_SHIFT) & MASTER_DIV_MASK;

	rate /= characteristics->divisors[div];

	if (rate < characteristics->output.min)
		pr_warn("master clk div is underclocked");
	else if (rate > characteristics->output.max)
		pr_warn("master clk div is overclocked");

	return rate;
}

static const struct clk_ops master_div_ops = {
	.enable = clk_master_prepare,
	.is_enabled = clk_master_is_prepared,
	.recalc_rate = clk_master_div_recalc_rate,
};

static unsigned long clk_master_div_recalc_rate_chg(struct clk_hw *hw,
						    unsigned long parent_rate)
{
	struct clk_master *master = to_clk_master(hw);

	return DIV_ROUND_CLOSEST_ULL(parent_rate, master->div);
}

static const struct clk_ops master_div_ops_chg = {
	.enable = clk_master_prepare,
	.is_enabled = clk_master_is_prepared,
	.recalc_rate = clk_master_div_recalc_rate_chg,
};

static unsigned long clk_master_pres_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct clk_master *master = to_clk_master(hw);
	const struct clk_master_characteristics *characteristics =
						master->characteristics;
	unsigned long flags;
	unsigned int val, pres;

	spin_lock_irqsave(master->lock, flags);
	regmap_read(master->regmap, master->layout->offset, &val);
	spin_unlock_irqrestore(master->lock, flags);

	val &= master->layout->mask;
	pres = (val >> master->layout->pres_shift) & MASTER_PRES_MASK;
	if (pres == MASTER_PRES_MAX && characteristics->have_div3_pres)
		pres = 3;
	else
		pres = (1 << pres);

	return DIV_ROUND_CLOSEST_ULL(parent_rate, pres);
}

static int clk_master_pres_get_parent(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long flags;
	unsigned int mckr;

	spin_lock_irqsave(master->lock, flags);
	regmap_read(master->regmap, master->layout->offset, &mckr);
	spin_unlock_irqrestore(master->lock, flags);

	mckr &= master->layout->mask;

	return mckr & AT91_PMC_CSS;
}

static const struct clk_ops master_pres_ops = {
	.enable = clk_master_prepare,
	.is_enabled = clk_master_is_prepared,
	.recalc_rate = clk_master_pres_recalc_rate,
	.get_parent = clk_master_pres_get_parent,
};

static struct clk_hw * __init
at91_clk_register_master_internal(struct regmap *regmap,
		const char *name, int num_parents,
		const char **parent_names,
		const struct clk_master_layout *layout,
		const struct clk_master_characteristics *characteristics,
		const struct clk_ops *ops, spinlock_t *lock, u32 flags)
{
	struct clk_master *master;
	struct clk_init_data init;
	struct clk_hw *hw;
	unsigned int mckr;
	unsigned long irqflags;
	int ret;

	if (!name || !num_parents || !parent_names || !lock)
		return ERR_PTR(-EINVAL);

	master = kzalloc(sizeof(*master), GFP_KERNEL);
	if (!master)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = ops;
	init.parent_names = parent_names;
	init.num_parents = num_parents;
	init.flags = flags;

	master->hw.init = &init;
	master->layout = layout;
	master->characteristics = characteristics;
	master->regmap = regmap;
	master->lock = lock;

	if (ops == &master_div_ops_chg) {
		spin_lock_irqsave(master->lock, irqflags);
		regmap_read(master->regmap, master->layout->offset, &mckr);
		spin_unlock_irqrestore(master->lock, irqflags);

		mckr &= layout->mask;
		mckr = (mckr >> MASTER_DIV_SHIFT) & MASTER_DIV_MASK;
		master->div = characteristics->divisors[mckr];
	}

	hw = &master->hw;
	ret = clk_hw_register(NULL, &master->hw);
	if (ret) {
		kfree(master);
		hw = ERR_PTR(ret);
	}

	return hw;
}

struct clk_hw * __init
at91_clk_register_master_pres(struct regmap *regmap,
		const char *name, int num_parents,
		const char **parent_names,
		const struct clk_master_layout *layout,
		const struct clk_master_characteristics *characteristics,
		spinlock_t *lock)
{
	return at91_clk_register_master_internal(regmap, name, num_parents,
						 parent_names, layout,
						 characteristics,
						 &master_pres_ops,
						 lock, CLK_SET_RATE_GATE);
}

struct clk_hw * __init
at91_clk_register_master_div(struct regmap *regmap,
		const char *name, const char *parent_name,
		const struct clk_master_layout *layout,
		const struct clk_master_characteristics *characteristics,
		spinlock_t *lock, u32 flags)
{
	const struct clk_ops *ops;

	if (flags & CLK_SET_RATE_GATE)
		ops = &master_div_ops;
	else
		ops = &master_div_ops_chg;

	return at91_clk_register_master_internal(regmap, name, 1,
						 &parent_name, layout,
						 characteristics, ops,
						 lock, flags);
}

static unsigned long
clk_sama7g5_master_recalc_rate(struct clk_hw *hw,
			       unsigned long parent_rate)
{
	struct clk_master *master = to_clk_master(hw);

	return DIV_ROUND_CLOSEST_ULL(parent_rate, (1 << master->div));
}

static int clk_sama7g5_master_get_parent(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long flags;
	u8 index;

	spin_lock_irqsave(master->lock, flags);
	index = clk_mux_val_to_index(&master->hw, master->mux_table, 0,
				     master->parent);
	spin_unlock_irqrestore(master->lock, flags);

	return index;
}

static int clk_sama7g5_master_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long flags;

	if (index >= clk_hw_get_num_parents(hw))
		return -EINVAL;

	spin_lock_irqsave(master->lock, flags);
	master->parent = clk_mux_index_to_val(master->mux_table, 0, index);
	spin_unlock_irqrestore(master->lock, flags);

	return 0;
}

static void clk_sama7g5_master_set(struct clk_master *master,
				   unsigned int status)
{
	unsigned long flags;
	unsigned int val, cparent;
	unsigned int enable = status ? AT91_PMC_MCR_V2_EN : 0;
	unsigned int parent = master->parent << PMC_MCR_CSS_SHIFT;
	unsigned int div = master->div << MASTER_DIV_SHIFT;

	spin_lock_irqsave(master->lock, flags);

	regmap_write(master->regmap, AT91_PMC_MCR_V2,
		     AT91_PMC_MCR_V2_ID(master->id));
	regmap_read(master->regmap, AT91_PMC_MCR_V2, &val);
	regmap_update_bits(master->regmap, AT91_PMC_MCR_V2,
			   enable | AT91_PMC_MCR_V2_CSS | AT91_PMC_MCR_V2_DIV |
			   AT91_PMC_MCR_V2_CMD | AT91_PMC_MCR_V2_ID_MSK,
			   enable | parent | div | AT91_PMC_MCR_V2_CMD |
			   AT91_PMC_MCR_V2_ID(master->id));

	cparent = (val & AT91_PMC_MCR_V2_CSS) >> PMC_MCR_CSS_SHIFT;

	/* Wait here only if parent is being changed. */
	while ((cparent != master->parent) && !clk_master_ready(master))
		cpu_relax();

	spin_unlock_irqrestore(master->lock, flags);
}

static int clk_sama7g5_master_enable(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);

	clk_sama7g5_master_set(master, 1);

	return 0;
}

static void clk_sama7g5_master_disable(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long flags;

	spin_lock_irqsave(master->lock, flags);

	regmap_write(master->regmap, AT91_PMC_MCR_V2, master->id);
	regmap_update_bits(master->regmap, AT91_PMC_MCR_V2,
			   AT91_PMC_MCR_V2_EN | AT91_PMC_MCR_V2_CMD |
			   AT91_PMC_MCR_V2_ID_MSK,
			   AT91_PMC_MCR_V2_CMD |
			   AT91_PMC_MCR_V2_ID(master->id));

	spin_unlock_irqrestore(master->lock, flags);
}

static int clk_sama7g5_master_is_enabled(struct clk_hw *hw)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long flags;
	unsigned int val;

	spin_lock_irqsave(master->lock, flags);

	regmap_write(master->regmap, AT91_PMC_MCR_V2, master->id);
	regmap_read(master->regmap, AT91_PMC_MCR_V2, &val);

	spin_unlock_irqrestore(master->lock, flags);

	return !!(val & AT91_PMC_MCR_V2_EN);
}

static int clk_sama7g5_master_set_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long parent_rate)
{
	struct clk_master *master = to_clk_master(hw);
	unsigned long div, flags;

	div = DIV_ROUND_CLOSEST(parent_rate, rate);
	if ((div > (1 << (MASTER_PRES_MAX - 1))) || (div & (div - 1)))
		return -EINVAL;

	if (div == 3)
		div = MASTER_PRES_MAX;
	else if (div)
		div = ffs(div) - 1;

	spin_lock_irqsave(master->lock, flags);
	master->div = div;
	spin_unlock_irqrestore(master->lock, flags);

	return 0;
}

static const struct clk_ops sama7g5_master_ops = {
	.enable = clk_sama7g5_master_enable,
	.disable = clk_sama7g5_master_disable,
	.is_enabled = clk_sama7g5_master_is_enabled,
	.recalc_rate = clk_sama7g5_master_recalc_rate,
	.set_rate = clk_sama7g5_master_set_rate,
	.get_parent = clk_sama7g5_master_get_parent,
	.set_parent = clk_sama7g5_master_set_parent,
};

struct clk_hw * __init
at91_clk_sama7g5_register_master(struct regmap *regmap,
				 const char *name, int num_parents,
				 const char **parent_names,
				 u32 *mux_table,
				 spinlock_t *lock, u8 id,
				 bool critical, int chg_pid)
{
	struct clk_master *master;
	struct clk_hw *hw;
	struct clk_init_data init;
	unsigned long flags;
	unsigned int val;
	int ret;

	if (!name || !num_parents || !parent_names || !mux_table ||
	    !lock || id > MASTER_MAX_ID)
		return ERR_PTR(-EINVAL);

	master = kzalloc(sizeof(*master), GFP_KERNEL);
	if (!master)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = &sama7g5_master_ops;
	init.parent_names = parent_names;
	init.num_parents = num_parents;
	init.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE;
	if (chg_pid >= 0)
		init.flags |= CLK_SET_RATE_PARENT;
	if (critical)
		init.flags |= CLK_IS_CRITICAL;

	master->hw.init = &init;
	master->regmap = regmap;
	master->id = id;
	master->chg_pid = chg_pid;
	master->lock = lock;
	master->mux_table = mux_table;

	spin_lock_irqsave(master->lock, flags);
	regmap_write(master->regmap, AT91_PMC_MCR_V2, master->id);
	regmap_read(master->regmap, AT91_PMC_MCR_V2, &val);
	master->parent = (val & AT91_PMC_MCR_V2_CSS) >> PMC_MCR_CSS_SHIFT;
	master->div = (val & AT91_PMC_MCR_V2_DIV) >> MASTER_DIV_SHIFT;
	spin_unlock_irqrestore(master->lock, flags);

	hw = &master->hw;
	ret = clk_hw_register(NULL, &master->hw);
	if (ret) {
		kfree(master);
		hw = ERR_PTR(ret);
	}

	return hw;
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

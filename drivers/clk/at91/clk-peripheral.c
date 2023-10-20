// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */

#include <common.h>
#include <clock.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>

#include "pmc.h"

#define PERIPHERAL_ID_MIN	2
#define PERIPHERAL_ID_MAX	31
#define PERIPHERAL_MASK(id)	(1 << ((id) & PERIPHERAL_ID_MAX))

#define PERIPHERAL_RSHIFT_MASK	0x3
#define PERIPHERAL_RSHIFT(val)	(((val) >> 16) & PERIPHERAL_RSHIFT_MASK)

#define PERIPHERAL_MAX_SHIFT	3

struct clk_peripheral {
	struct clk_hw hw;
	struct regmap *regmap;
	u32 id;
	const char *parent;
};

#define to_clk_peripheral(_hw) container_of(_hw, struct clk_peripheral, hw)

struct clk_sam9x5_peripheral {
	struct clk_hw hw;
	struct regmap *regmap;
	struct clk_range range;
	u32 id;
	u32 div;
	const struct clk_pcr_layout *layout;
	bool auto_div;
	const char *parent;
};

#define to_clk_sam9x5_peripheral(_hw) \
	container_of(_hw, struct clk_sam9x5_peripheral, hw)

static int clk_peripheral_enable(struct clk_hw *hw)
{
	struct clk_peripheral *periph = to_clk_peripheral(hw);
	int offset = AT91_PMC_PCER;
	u32 id = periph->id;

	if (id < PERIPHERAL_ID_MIN)
		return 0;
	if (id > PERIPHERAL_ID_MAX)
		offset = AT91_PMC_PCER1;
	regmap_write(periph->regmap, offset, PERIPHERAL_MASK(id));

	return 0;
}

static void clk_peripheral_disable(struct clk_hw *hw)
{
	struct clk_peripheral *periph = to_clk_peripheral(hw);
	int offset = AT91_PMC_PCDR;
	u32 id = periph->id;

	if (id < PERIPHERAL_ID_MIN)
		return;
	if (id > PERIPHERAL_ID_MAX)
		offset = AT91_PMC_PCDR1;
	regmap_write(periph->regmap, offset, PERIPHERAL_MASK(id));
}

static int clk_peripheral_is_enabled(struct clk_hw *hw)
{
	struct clk_peripheral *periph = to_clk_peripheral(hw);
	int offset = AT91_PMC_PCSR;
	unsigned int status;
	u32 id = periph->id;

	if (id < PERIPHERAL_ID_MIN)
		return 1;
	if (id > PERIPHERAL_ID_MAX)
		offset = AT91_PMC_PCSR1;
	regmap_read(periph->regmap, offset, &status);

	return status & PERIPHERAL_MASK(id) ? 1 : 0;
}

static const struct clk_ops peripheral_ops = {
	.enable = clk_peripheral_enable,
	.disable = clk_peripheral_disable,
	.is_enabled = clk_peripheral_is_enabled,
};

struct clk * __init
at91_clk_register_peripheral(struct regmap *regmap, const char *name,
			     const char *parent_name, u32 id)
{
	int ret;
	struct clk_peripheral *periph;

	if (!name || !parent_name || id > PERIPHERAL_ID_MAX)
		return ERR_PTR(-EINVAL);

	periph = xzalloc(sizeof(*periph));

	periph->hw.clk.name = name;
	periph->hw.clk.ops = &peripheral_ops;

	if (parent_name) {
		periph->parent = parent_name;
		periph->hw.clk.parent_names = &periph->parent;
		periph->hw.clk.num_parents = 1;
	}

	periph->id = id;
	periph->regmap = regmap;

	ret = bclk_register(&periph->hw.clk);
	if (ret) {
		kfree(periph);
		return ERR_PTR(ret);
	}

	return &periph->hw.clk;
}

static void clk_sam9x5_peripheral_autodiv(struct clk_sam9x5_peripheral *periph)
{
	struct clk *parent;
	unsigned long parent_rate;
	int shift = 0;

	if (!periph->auto_div)
		return;

	if (periph->range.max) {
		parent = clk_get_parent(&periph->hw.clk);
		parent_rate = clk_get_rate(parent);
		if (!parent_rate)
			return;

		for (; shift < PERIPHERAL_MAX_SHIFT; shift++) {
			if (parent_rate >> shift <= periph->range.max)
				break;
		}
	}

	periph->auto_div = false;
	periph->div = shift;
}

static int clk_sam9x5_peripheral_enable(struct clk_hw *hw)
{
	struct clk_sam9x5_peripheral *periph = to_clk_sam9x5_peripheral(hw);

	if (periph->id < PERIPHERAL_ID_MIN)
		return 0;

	regmap_write(periph->regmap, periph->layout->offset,
		     (periph->id & periph->layout->pid_mask));
	regmap_write_bits(periph->regmap, periph->layout->offset,
			  periph->layout->div_mask | periph->layout->cmd |
			  AT91_PMC_PCR_EN,
			  field_prep(periph->layout->div_mask, periph->div) |
			  periph->layout->cmd |
			  AT91_PMC_PCR_EN);

	return 0;
}

static void clk_sam9x5_peripheral_disable(struct clk_hw *hw)
{
	struct clk_sam9x5_peripheral *periph = to_clk_sam9x5_peripheral(hw);

	if (periph->id < PERIPHERAL_ID_MIN)
		return;

	regmap_write(periph->regmap, periph->layout->offset,
		     (periph->id & periph->layout->pid_mask));
	regmap_write_bits(periph->regmap, periph->layout->offset,
			  AT91_PMC_PCR_EN | periph->layout->cmd,
			  periph->layout->cmd);
}

static int clk_sam9x5_peripheral_is_enabled(struct clk_hw *hw)
{
	struct clk_sam9x5_peripheral *periph = to_clk_sam9x5_peripheral(hw);
	unsigned int status;

	if (periph->id < PERIPHERAL_ID_MIN)
		return 1;

	regmap_write(periph->regmap, periph->layout->offset,
		     (periph->id & periph->layout->pid_mask));
	regmap_read(periph->regmap, periph->layout->offset, &status);

	return status & AT91_PMC_PCR_EN ? 1 : 0;
}

static unsigned long
clk_sam9x5_peripheral_recalc_rate(struct clk_hw *hw,
				  unsigned long parent_rate)
{
	struct clk_sam9x5_peripheral *periph = to_clk_sam9x5_peripheral(hw);
	unsigned int status;

	if (periph->id < PERIPHERAL_ID_MIN)
		return parent_rate;

	regmap_write(periph->regmap, periph->layout->offset,
		     (periph->id & periph->layout->pid_mask));
	regmap_read(periph->regmap, periph->layout->offset, &status);

	if (status & AT91_PMC_PCR_EN) {
		periph->div = field_get(periph->layout->div_mask, status);
		periph->auto_div = false;
	} else {
		clk_sam9x5_peripheral_autodiv(periph);
	}

	return parent_rate >> periph->div;
}

static long clk_sam9x5_peripheral_round_rate(struct clk_hw *hw,
					     unsigned long rate,
					     unsigned long *parent_rate)
{
	int shift = 0;
	unsigned long best_rate;
	unsigned long best_diff;
	unsigned long cur_rate = *parent_rate;
	unsigned long cur_diff;
	struct clk_sam9x5_peripheral *periph = to_clk_sam9x5_peripheral(hw);

	if (periph->id < PERIPHERAL_ID_MIN || !periph->range.max)
		return *parent_rate;

	if (periph->range.max) {
		for (; shift <= PERIPHERAL_MAX_SHIFT; shift++) {
			cur_rate = *parent_rate >> shift;
			if (cur_rate <= periph->range.max)
				break;
		}
	}

	if (rate >= cur_rate)
		return cur_rate;

	best_diff = cur_rate - rate;
	best_rate = cur_rate;
	for (; shift <= PERIPHERAL_MAX_SHIFT; shift++) {
		cur_rate = *parent_rate >> shift;
		if (cur_rate < rate)
			cur_diff = rate - cur_rate;
		else
			cur_diff = cur_rate - rate;

		if (cur_diff < best_diff) {
			best_diff = cur_diff;
			best_rate = cur_rate;
		}

		if (!best_diff || cur_rate < rate)
			break;
	}

	return best_rate;
}

static int clk_sam9x5_peripheral_set_rate(struct clk_hw *hw,
					  unsigned long rate,
					  unsigned long parent_rate)
{
	int shift;
	struct clk_sam9x5_peripheral *periph = to_clk_sam9x5_peripheral(hw);
	if (periph->id < PERIPHERAL_ID_MIN || !periph->range.max) {
		if (parent_rate == rate)
			return 0;
		else
			return -EINVAL;
	}

	if (periph->range.max && rate > periph->range.max)
		return -EINVAL;

	for (shift = 0; shift <= PERIPHERAL_MAX_SHIFT; shift++) {
		if (parent_rate >> shift == rate) {
			periph->auto_div = false;
			periph->div = shift;
			return 0;
		}
	}

	return -EINVAL;
}

static const struct clk_ops sam9x5_peripheral_ops = {
	.enable = clk_sam9x5_peripheral_enable,
	.disable = clk_sam9x5_peripheral_disable,
	.is_enabled = clk_sam9x5_peripheral_is_enabled,
	.recalc_rate = clk_sam9x5_peripheral_recalc_rate,
	.round_rate = clk_sam9x5_peripheral_round_rate,
	.set_rate = clk_sam9x5_peripheral_set_rate,
};

struct clk * __init
at91_clk_register_sam9x5_peripheral(struct regmap *regmap,
				    const struct clk_pcr_layout *layout,
				    const char *name, const char *parent_name,
				    u32 id, const struct clk_range *range)
{
	int ret;
	struct clk_sam9x5_peripheral *periph;

	if (!name || !parent_name)
		return ERR_PTR(-EINVAL);

	periph = xzalloc(sizeof(*periph));

	periph->hw.clk.name = name;
	periph->hw.clk.ops = &sam9x5_peripheral_ops;

	if (parent_name) {
		periph->parent = parent_name;
		periph->hw.clk.parent_names = &periph->parent;
		periph->hw.clk.num_parents = 1;
	}

	periph->id = id;
	periph->div = 0;
	periph->regmap = regmap;
	if (layout->div_mask)
		periph->auto_div = true;
	periph->layout = layout;
	periph->range = *range;

	ret = bclk_register(&periph->hw.clk);
	if (ret) {
		kfree(periph);
		return ERR_PTR(ret);
	}

	clk_sam9x5_peripheral_autodiv(periph);
	pmc_register_id(id);

	return &periph->hw.clk;
}

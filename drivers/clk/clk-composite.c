// SPDX-License-Identifier: GPL-2.0-only
/*
 * Taken from linux/drivers/clk/
 *
 * Copyright (c) 2013 NVIDIA CORPORATION.  All rights reserved.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

struct clk_composite {
	struct clk_hw	hw;

	struct clk	*mux_clk;
	struct clk	*rate_clk;
	struct clk	*gate_clk;
};

#define to_clk_composite(_hw) container_of(_hw, struct clk_composite, hw)

static int clk_composite_get_parent(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *mux_clk = composite->mux_clk;
	struct clk_hw *mux_hw = clk_to_clk_hw(mux_clk);

	return mux_clk ? mux_clk->ops->get_parent(mux_hw) : 0;
}

static int clk_composite_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *mux_clk = composite->mux_clk;
	struct clk_hw *mux_hw = clk_to_clk_hw(mux_clk);

	return mux_clk ? mux_clk->ops->set_parent(mux_hw, index) : 0;
}

static unsigned long clk_composite_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *rate_clk = composite->rate_clk;
	struct clk_hw *rate_hw = clk_to_clk_hw(rate_clk);

	if (rate_clk)
		return rate_clk->ops->recalc_rate(rate_hw, parent_rate);

	return parent_rate;
}

static long clk_composite_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *prate)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *rate_clk = composite->rate_clk;
	struct clk *mux_clk = composite->mux_clk;
	struct clk_hw *rate_hw = clk_to_clk_hw(rate_clk);

	if (rate_clk)
		return rate_clk->ops->round_rate(rate_hw, rate, prate);

	if (!(hw->clk.flags & CLK_SET_RATE_NO_REPARENT) &&
	    mux_clk &&
	    mux_clk->ops->round_rate)
		return mux_clk->ops->round_rate(clk_to_clk_hw(mux_clk), rate, prate);

	return *prate;
}

static int clk_composite_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *rate_clk = composite->rate_clk;
	struct clk *mux_clk = composite->mux_clk;
	struct clk_hw *rate_hw = clk_to_clk_hw(rate_clk);

	/*
	 * When the rate clock is present use that to set the rate,
	 * otherwise try the mux clock. We currently do not support
	 * to find the best rate using a combination of both.
	 */
	if (rate_clk)
		return rate_clk->ops->set_rate(rate_hw, rate, parent_rate);

	if (!(hw->clk.flags & CLK_SET_RATE_NO_REPARENT) &&
	    mux_clk &&
	    mux_clk->ops->set_rate) {
		/*
		 * We'll call set_rate on the mux clk which in turn results
		 * in reparenting the mux clk. Make sure the enable count
		 * (which is stored in the composite clk, not the mux clk)
		 * is transferred correctly.
		 */
		mux_clk->enable_count = hw->clk.enable_count;
		return mux_clk->ops->set_rate(clk_to_clk_hw(mux_clk), rate, parent_rate);
	}

	return 0;
}

static int clk_composite_is_enabled(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *gate_clk = composite->gate_clk;
	struct clk_hw *gate_hw = clk_to_clk_hw(gate_clk);

	return gate_clk ? gate_clk->ops->is_enabled(gate_hw) : 0;
}

static int clk_composite_enable(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *gate_clk = composite->gate_clk;
	struct clk_hw *gate_hw = clk_to_clk_hw(gate_clk);

	return gate_clk ? gate_clk->ops->enable(gate_hw) : 0;
}

static void clk_composite_disable(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk *gate_clk = composite->gate_clk;
	struct clk_hw *gate_hw = clk_to_clk_hw(gate_clk);

	if (gate_clk)
		gate_clk->ops->disable(gate_hw);
}

static struct clk_ops clk_composite_ops = {
	.get_parent = clk_composite_get_parent,
	.set_parent = clk_composite_set_parent,
	.recalc_rate = clk_composite_recalc_rate,
	.round_rate = clk_composite_round_rate,
	.set_rate = clk_composite_set_rate,
	.is_enabled = clk_composite_is_enabled,
	.enable = clk_composite_enable,
	.disable = clk_composite_disable,
};

struct clk *clk_register_composite(const char *name,
			const char * const *parent_names, int num_parents,
			struct clk *mux_clk,
			struct clk *rate_clk,
			struct clk *gate_clk,
			unsigned long flags)
{
	struct clk_composite *composite;
	int ret;

	composite = xzalloc(sizeof(*composite));

	composite->hw.clk.name = name;
	composite->hw.clk.ops = &clk_composite_ops;
	composite->hw.clk.flags = flags;
	composite->hw.clk.parent_names = parent_names;
	composite->hw.clk.num_parents = num_parents;
	composite->mux_clk = mux_clk;
	composite->rate_clk = rate_clk;
	composite->gate_clk = gate_clk;

	ret = bclk_register(&composite->hw.clk);
	if (ret)
		goto err;

	if (composite->mux_clk) {
		composite->mux_clk->parents = composite->hw.clk.parents;
		composite->mux_clk->parent_names = composite->hw.clk.parent_names;
		composite->mux_clk->num_parents = composite->hw.clk.num_parents;
	}

	return &composite->hw.clk;

err:
	kfree(composite);
	return 0;
}

struct clk_hw *clk_hw_register_composite(struct device *dev,
					 const char *name,
					 const char * const *parent_names,
					 int num_parents,
					 struct clk_hw *mux_hw,
					 const struct clk_ops *mux_ops,
					 struct clk_hw *rate_hw,
					 const struct clk_ops *rate_ops,
					 struct clk_hw *gate_hw,
					 const struct clk_ops *gate_ops,
					 unsigned long flags)
{
	struct clk *clk;

	if (mux_hw)
		mux_hw->clk.ops = mux_ops;
	if (rate_hw)
		rate_hw->clk.ops = rate_ops;
	if (gate_hw)
		gate_hw->clk.ops = gate_ops;

	parent_names = memdup_array(parent_names, num_parents);
	if (!parent_names)
		return ERR_PTR(-ENOMEM);

	clk = clk_register_composite(xstrdup(name), parent_names, num_parents,
				      mux_hw ? &mux_hw->clk : NULL,
				      rate_hw ? &rate_hw->clk : NULL,
				      gate_hw ? &gate_hw->clk : NULL,
				      flags);
	return clk_to_clk_hw(clk);
}

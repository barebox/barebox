// SPDX-License-Identifier: GPL-2.0-only
/*
 * Taken from linux/drivers/clk/
 *
 * Copyright (c) 2013 NVIDIA CORPORATION.  All rights reserved.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk-provider.h>
#include <linux/err.h>

static int clk_composite_get_parent(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *mux_hw = composite->mux_hw;

	return mux_hw ? mux_hw->clk.ops->get_parent(mux_hw) : 0;
}

static int clk_composite_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *mux_hw = composite->mux_hw;

	return mux_hw ? mux_hw->clk.ops->set_parent(mux_hw, index) : 0;
}

static unsigned long clk_composite_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *rate_hw = composite->rate_hw;

	if (rate_hw)
		return rate_hw->clk.ops->recalc_rate(rate_hw, parent_rate);

	return parent_rate;
}

static int clk_composite_determine_rate_for_parent(struct clk_hw *rate_hw,
						   struct clk_rate_request *req,
						   struct clk_hw *parent_hw)
{
	req->best_parent_hw = parent_hw;
	req->best_parent_rate = clk_hw_get_rate(parent_hw);

	return rate_hw->clk.ops->determine_rate(rate_hw, req);
}

static int clk_composite_determine_rate(struct clk_hw *hw,
					struct clk_rate_request *req)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *rate_hw = composite->rate_hw;
	struct clk_hw *mux_hw = composite->mux_hw;
	struct clk_hw *parent;
	unsigned long rate_diff;
	unsigned long best_rate_diff = ULONG_MAX;
	unsigned long best_rate = 0;
	int i, ret;

	/*
	 * When both a mux and a rate clock are present, iterate all mux
	 * parents and ask the rate clock for the best rate it can achieve
	 * with each, then pick the (parent, rate) pair closest to the
	 * request.  Trying only one of the two independently would miss
	 * the globally optimal combination.
	 *
	 * Note: Linux uses __clk_hw_set_clk() so the rate clock inherits
	 * the composite's flags (including CLK_SET_RATE_PARENT).  barebox
	 * has no clk_core abstraction, so the rate clock retains its own
	 * flags.  If the composite has CLK_SET_RATE_PARENT but the rate
	 * clock doesn't, the rate clock won't negotiate parent rates in
	 * clk_divider_bestdiv().  The best (parent, divider) pair is still
	 * found using each parent's current rate, which is the common case.
	 */
	if (rate_hw && rate_hw->clk.ops->determine_rate &&
	    mux_hw && mux_hw->clk.ops->set_parent) {
		req->best_parent_hw = NULL;

		if (hw->clk.flags & CLK_SET_RATE_NO_REPARENT) {
			struct clk_rate_request tmp_req;

			parent = clk_hw_get_parent(hw);

			clk_hw_forward_rate_request(hw, req, parent,
						    &tmp_req, req->rate);
			ret = clk_composite_determine_rate_for_parent(
					rate_hw, &tmp_req, parent);
			if (ret)
				return ret;

			req->rate = tmp_req.rate;
			req->best_parent_hw = tmp_req.best_parent_hw;
			req->best_parent_rate = tmp_req.best_parent_rate;

			return 0;
		}

		for (i = 0; i < hw->clk.num_parents; i++) {
			struct clk_rate_request tmp_req;

			parent = clk_hw_get_parent_by_index(hw, i);
			if (!parent)
				continue;

			clk_hw_forward_rate_request(hw, req, parent,
						    &tmp_req, req->rate);
			ret = clk_composite_determine_rate_for_parent(
					rate_hw, &tmp_req, parent);
			if (ret)
				continue;

			if (req->rate >= tmp_req.rate)
				rate_diff = req->rate - tmp_req.rate;
			else
				rate_diff = tmp_req.rate - req->rate;

			if (!rate_diff || !req->best_parent_hw
				       || best_rate_diff > rate_diff) {
				req->best_parent_hw = parent;
				req->best_parent_rate =
					tmp_req.best_parent_rate;
				best_rate_diff = rate_diff;
				best_rate = tmp_req.rate;
			}

			if (!rate_diff)
				return 0;
		}

		req->rate = best_rate;
		return 0;
	}

	/*
	 * Otherwise delegate directly to the sub-clock ops, passing
	 * through the same req.  This works because composite sub-clocks
	 * share the composite's parent topology, so req->best_parent_hw
	 * and req->best_parent_rate remain valid for the sub-clock.
	 */
	if (rate_hw && rate_hw->clk.ops->determine_rate)
		return rate_hw->clk.ops->determine_rate(rate_hw, req);

	if (!(hw->clk.flags & CLK_SET_RATE_NO_REPARENT) && mux_hw &&
	    mux_hw->clk.ops->determine_rate)
		return mux_hw->clk.ops->determine_rate(mux_hw, req);

	req->rate = req->best_parent_rate;
	return 0;
}

static int clk_composite_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *rate_hw = composite->rate_hw;

	/*
	 * Linux assembles its clk_composite_ops at registration time and
	 * only installs set_rate when the rate clock has one. barebox'
	 * clk_composite_ops is static, so check here instead: a rate clock
	 * without set_rate has nothing to program.
	 */
	if (rate_hw && rate_hw->clk.ops->set_rate)
		return rate_hw->clk.ops->set_rate(rate_hw, rate, parent_rate);

	return 0;
}

static int clk_composite_is_enabled(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *gate_hw = composite->gate_hw;

	return gate_hw ? gate_hw->clk.ops->is_enabled(gate_hw) : 0;
}

static int clk_composite_enable(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *gate_hw = composite->gate_hw;

	return gate_hw ? gate_hw->clk.ops->enable(gate_hw) : 0;
}

static void clk_composite_disable(struct clk_hw *hw)
{
	struct clk_composite *composite = to_clk_composite(hw);
	struct clk_hw *gate_hw = composite->gate_hw;

	if (gate_hw)
		gate_hw->clk.ops->disable(gate_hw);
}

static struct clk_ops clk_composite_ops = {
	.get_parent = clk_composite_get_parent,
	.set_parent = clk_composite_set_parent,
	.recalc_rate = clk_composite_recalc_rate,
	.determine_rate = clk_composite_determine_rate,
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
	composite->mux_hw = clk_to_clk_hw(mux_clk);
	composite->rate_hw = clk_to_clk_hw(rate_clk);
	composite->gate_hw = clk_to_clk_hw(gate_clk);

	ret = bclk_register(&composite->hw.clk);
	if (ret)
		goto err;

	if (composite->mux_hw) {
		composite->mux_hw->clk.parents = composite->hw.clk.parents;
		composite->mux_hw->clk.parent_names = composite->hw.clk.parent_names;
		composite->mux_hw->clk.num_parents = composite->hw.clk.num_parents;
	}

	return &composite->hw.clk;

err:
	kfree(composite);
	return ERR_PTR(ret);
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

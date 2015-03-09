/*
 * Taken from linux/drivers/clk/
 *
 * Copyright (c) 2013 NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

struct clk_composite {
	struct clk	clk;

	struct clk	*mux_clk;
	struct clk	*rate_clk;
	struct clk	*gate_clk;
};

#define to_clk_composite(_clk) container_of(_clk, struct clk_composite, clk)

static int clk_composite_get_parent(struct clk *clk)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *mux_clk = composite->mux_clk;

	return mux_clk ? mux_clk->ops->get_parent(mux_clk) : 0;
}

static int clk_composite_set_parent(struct clk *clk, u8 index)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *mux_clk = composite->mux_clk;

	return mux_clk ? mux_clk->ops->set_parent(mux_clk, index) : 0;
}

static unsigned long clk_composite_recalc_rate(struct clk *clk,
					    unsigned long parent_rate)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *rate_clk = composite->rate_clk;

	return rate_clk ? rate_clk->ops->recalc_rate(rate_clk, parent_rate) : 0;
}

static long clk_composite_round_rate(struct clk *clk, unsigned long rate,
				  unsigned long *prate)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *rate_clk = composite->rate_clk;

	return rate_clk ? rate_clk->ops->round_rate(rate_clk, rate, prate) : 0;
}

static int clk_composite_set_rate(struct clk *clk, unsigned long rate,
			       unsigned long parent_rate)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *rate_clk = composite->rate_clk;

	return rate_clk ?
		rate_clk->ops->set_rate(rate_clk, rate, parent_rate) : 0;
}

static int clk_composite_is_enabled(struct clk *clk)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *gate_clk = composite->gate_clk;

	return gate_clk ? gate_clk->ops->is_enabled(gate_clk) : 0;
}

static int clk_composite_enable(struct clk *clk)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *gate_clk = composite->gate_clk;

	return gate_clk ? gate_clk->ops->enable(gate_clk) : 0;
}

static void clk_composite_disable(struct clk *clk)
{
	struct clk_composite *composite = to_clk_composite(clk);
	struct clk *gate_clk = composite->gate_clk;

	if (gate_clk)
		gate_clk->ops->disable(gate_clk);
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
			const char **parent_names, int num_parents,
			struct clk *mux_clk,
			struct clk *rate_clk,
			struct clk *gate_clk,
			unsigned long flags)
{
	struct clk_composite *composite;
	int ret;

	composite = xzalloc(sizeof(*composite));

	composite->clk.name = name;
	composite->clk.ops = &clk_composite_ops;
	composite->clk.flags = flags;
	composite->clk.parent_names = parent_names;
	composite->clk.num_parents = num_parents;
	composite->mux_clk = mux_clk;
	composite->rate_clk = rate_clk;
	composite->gate_clk = gate_clk;

	ret = clk_register(&composite->clk);
	if (ret)
		goto err;

	return &composite->clk;

err:
	kfree(composite);
	return 0;
}

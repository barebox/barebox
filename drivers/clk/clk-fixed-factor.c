// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-fixed-factor.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

static unsigned long clk_fixed_factor_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_fixed_factor *f = to_clk_fixed_factor(hw);

	return (parent_rate / f->div) * f->mult;
}

static long clk_factor_round_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long *prate)
{
	struct clk_fixed_factor *fix = to_clk_fixed_factor(hw);
	struct clk *clk = clk_hw_to_clk(hw);

	if (clk->flags & CLK_SET_RATE_PARENT) {
		unsigned long best_parent;

		best_parent = (rate / fix->mult) * fix->div;
		*prate = clk_round_rate(clk_get_parent(clk), best_parent);
	}

	return (*prate / fix->div) * fix->mult;
}

static int clk_factor_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_fixed_factor *fix = to_clk_fixed_factor(hw);
	struct clk *clk = clk_hw_to_clk(hw);

	if (clk->flags & CLK_SET_RATE_PARENT) {
		return clk_set_rate(clk_get_parent(clk), rate * fix->div / fix->mult);
	}

	return 0;
}

struct clk_ops clk_fixed_factor_ops = {
	.set_rate = clk_factor_set_rate,
	.round_rate = clk_factor_round_rate,
	.recalc_rate = clk_fixed_factor_recalc_rate,
};

struct clk *clk_fixed_factor(const char *name,
		const char *parent, unsigned int mult, unsigned int div, unsigned flags)
{
	struct clk_fixed_factor *f = xzalloc(sizeof(*f));
	int ret;

	f->mult = mult;
	f->div = div;
	f->parent = parent;
	f->hw.clk.ops = &clk_fixed_factor_ops;
	f->hw.clk.name = name;
	f->hw.clk.flags = flags;
	f->hw.clk.parent_names = &f->parent;
	f->hw.clk.num_parents = 1;

	ret = bclk_register(&f->hw.clk);
	if (ret) {
		free(f);
		return ERR_PTR(ret);
	}

	return &f->hw.clk;
}

struct clk *clk_register_fixed_factor(struct device_d *dev, const char *name,
		const char *parent_name, unsigned long flags,
		unsigned int mult, unsigned int div)
{
	return clk_fixed_factor(name, parent_name, mult, div, flags);
}

struct clk_hw *clk_hw_register_fixed_factor(struct device_d *dev,
		const char *name, const char *parent_name, unsigned long flags,
		unsigned int mult, unsigned int div)
{
	return clk_to_clk_hw(clk_register_fixed_factor(dev, xstrdup(name),
						       xstrdup(parent_name),
						       flags, mult, div));
}

/**
 * of_fixed_factor_clk_setup() - Setup function for simple fixed factor clock
 */
static int of_fixed_factor_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(node, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a clock-div property\n",
			__func__, node->name);
		return -EINVAL;
	}

	if (of_property_read_u32(node, "clock-mult", &mult)) {
		pr_err("%s Fixed factor clock <%s> must have a clock-mult property\n",
			__func__, node->name);
		return -EINVAL;
	}

	of_property_read_string(node, "clock-output-names", &clk_name);
	parent_name = of_clk_get_parent_name(node, 0);

	clk = clk_fixed_factor(clk_name, parent_name, mult, div, 0);
	if (IS_ERR(clk))
		return IS_ERR(clk);

	return of_clk_add_provider(node, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(fixed_factor_clk, "fixed-factor-clock",
		of_fixed_factor_clk_setup);

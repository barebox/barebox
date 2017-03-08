/*
 * clk-fixed-factor.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

struct clk_fixed_factor {
	struct clk clk;
	int mult;
	int div;
	const char *parent;
};

static unsigned long clk_fixed_factor_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	struct clk_fixed_factor *f = container_of(clk, struct clk_fixed_factor, clk);

	return (parent_rate / f->div) * f->mult;
}

static long clk_factor_round_rate(struct clk *clk, unsigned long rate,
		unsigned long *prate)
{
	struct clk_fixed_factor *fix = container_of(clk, struct clk_fixed_factor, clk);

	if (clk->flags & CLK_SET_RATE_PARENT) {
		unsigned long best_parent;

		best_parent = (rate / fix->mult) * fix->div;
		*prate = clk_round_rate(clk_get_parent(clk), best_parent);
	}

	return (*prate / fix->div) * fix->mult;
}

static int clk_factor_set_rate(struct clk *clk, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_fixed_factor *fix = container_of(clk, struct clk_fixed_factor, clk);

	if (clk->flags & CLK_SET_RATE_PARENT) {
		printk("%s: %ld -> parent %ld\n", __func__, rate, rate * fix->div / fix->mult);
		return clk_set_rate(clk_get_parent(clk), rate * fix->div / fix->mult);
	}

	return 0;
}

static struct clk_ops clk_fixed_factor_ops = {
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
	f->clk.ops = &clk_fixed_factor_ops;
	f->clk.name = name;
	f->clk.flags = flags;
	f->clk.parent_names = &f->parent;
	f->clk.num_parents = 1;

	ret = clk_register(&f->clk);
	if (ret) {
		free(f);
		return ERR_PTR(ret);
	}

	return &f->clk;
}

#if defined(CONFIG_COMMON_CLK_OF_PROVIDER)
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
#endif

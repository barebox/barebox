// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-fixed.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

struct clk_fixed {
	struct clk clk;
	unsigned long rate;
};

static unsigned long clk_fixed_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	struct clk_fixed *fix = container_of(clk, struct clk_fixed, clk);

	return fix->rate;
}

static struct clk_ops clk_fixed_ops = {
	.recalc_rate = clk_fixed_recalc_rate,
	.is_enabled = clk_is_enabled_always,
};

struct clk *clk_register_fixed_rate(const char *name,
				    const char *parent_name, unsigned long flags,
				    unsigned long rate)
{
	struct clk_fixed *fix = xzalloc(sizeof *fix);
	const char **parent_names = NULL;
	int ret;

	fix->rate = rate;
	fix->clk.ops = &clk_fixed_ops;
	fix->clk.name = name;
	fix->clk.flags = flags;

	if (parent_name) {
		parent_names = kzalloc(sizeof(const char *), GFP_KERNEL);
		if (!parent_names)
			return ERR_PTR(-ENOMEM);

		fix->clk.parent_names = parent_names;
		fix->clk.num_parents = 1;
	}

	ret = clk_register(&fix->clk);
	if (ret) {
		free(parent_names);
		free(fix);
		return ERR_PTR(ret);
	}

	return &fix->clk;
}

/**
 * of_fixed_clk_setup() - Setup function for simple fixed rate clock
 */
static int of_fixed_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	u32 rate;

	if (of_property_read_u32(node, "clock-frequency", &rate))
		return -EINVAL;

	of_property_read_string(node, "clock-output-names", &clk_name);

	clk = clk_fixed(clk_name, rate);
	if (IS_ERR(clk))
		return IS_ERR(clk);
	return of_clk_add_provider(node, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(fixed_clk, "fixed-clock", of_fixed_clk_setup);

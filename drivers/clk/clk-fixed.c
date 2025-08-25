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

#include "clk-fixed.h"

struct clk_fixed {
	struct clk_hw hw;
	unsigned long rate;
};

static unsigned long clk_fixed_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_fixed *fix = container_of(hw, struct clk_fixed, hw);

	return fix->rate;
}

static struct clk_ops clk_fixed_ops = {
	.recalc_rate = clk_fixed_recalc_rate,
	.is_enabled = clk_is_enabled_always,
};

bool clk_is_fixed(struct clk *clk)
{
	return clk->ops == &clk_fixed_ops;
}

struct clk *clk_register_fixed_rate(const char *name,
				    const char *parent_name, unsigned long flags,
				    unsigned long rate)
{
	struct clk_fixed *fix = xzalloc(sizeof *fix);
	const char **parent_names = NULL;
	int ret;

	fix->rate = rate;
	fix->hw.clk.ops = &clk_fixed_ops;
	fix->hw.clk.name = name;
	fix->hw.clk.flags = flags;

	if (parent_name) {
		parent_names = kzalloc(sizeof(const char *), GFP_KERNEL);
		if (!parent_names)
			return ERR_PTR(-ENOMEM);

		parent_names[0] = strdup(parent_name);
		if (!parent_names[0])
			return ERR_PTR(-ENOMEM);

		fix->hw.clk.parent_names = parent_names;
		fix->hw.clk.num_parents = 1;
	}

	ret = bclk_register(&fix->hw.clk);
	if (ret) {
		free(parent_names);
		free(fix);
		return ERR_PTR(ret);
	}

	return &fix->hw.clk;
}

struct clk_hw *clk_hw_register_fixed_rate(struct device *dev,
					  const char *name, const char *parent_name,
					  unsigned long flags, unsigned long rate)
{
	return clk_to_clk_hw(clk_register_fixed_rate(xstrdup(name), parent_name,
						     flags, rate));
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

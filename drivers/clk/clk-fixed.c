/*
 * clk-fixed.c - generic barebox clock support. Based on Linux clk support
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

struct clk *clk_fixed(const char *name, int rate)
{
	struct clk_fixed *fix = xzalloc(sizeof *fix);
	int ret;

	fix->rate = rate;
	fix->clk.ops = &clk_fixed_ops;
	fix->clk.name = name;

	ret = clk_register(&fix->clk);
	if (ret) {
		free(fix);
		return ERR_PTR(ret);
	}

	return &fix->clk;
}

#if defined(CONFIG_COMMON_CLK_OF_PROVIDER)
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
#endif

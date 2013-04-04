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

struct clk_ops clk_fixed_ops = {
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

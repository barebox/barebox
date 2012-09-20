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

struct clk_ops clk_fixed_factor_ops = {
	.recalc_rate = clk_fixed_factor_recalc_rate,
};

struct clk *clk_fixed_factor(const char *name,
		const char *parent, unsigned int mult, unsigned int div)
{
	struct clk_fixed_factor *f = xzalloc(sizeof(*f));
	int ret;

	f->mult = mult;
	f->div = div;
	f->parent = parent;
	f->clk.ops = &clk_fixed_factor_ops;
	f->clk.name = name;
	f->clk.parent_names = &f->parent;
	f->clk.num_parents = 1;

	ret = clk_register(&f->clk);
	if (ret) {
		free(f);
		return ERR_PTR(ret);
	}

	return &f->clk;
}

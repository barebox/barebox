/*
 * clk-divider.c - generic barebox clock support. Based on Linux clk support
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

static unsigned int clk_divider_maxdiv(struct clk_divider *div)
{
	if (div->flags & CLK_DIVIDER_ONE_BASED)
                return (1 << div->width) - 1;
	return 1 << div->width;
}

static int clk_divider_set_rate(struct clk *clk, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_divider *div = container_of(clk, struct clk_divider, clk);
	unsigned int val, divval;

	if (rate > parent_rate)
		rate = parent_rate;
	if (!rate)
		rate = 1;

	divval = DIV_ROUND_UP(parent_rate, rate);
	if (divval > clk_divider_maxdiv(div))
		divval = clk_divider_maxdiv(div);

	if (!(div->flags & CLK_DIVIDER_ONE_BASED))
		divval--;

	val = readl(div->reg);
	val &= ~(((1 << div->width) - 1) << div->shift);
	val |= divval << div->shift;
	writel(val, div->reg);

	return 0;
}

static unsigned long clk_divider_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	struct clk_divider *div = container_of(clk, struct clk_divider, clk);
	unsigned int val;

	val = readl(div->reg) >> div->shift;
	val &= (1 << div->width) - 1;

	if (div->flags & CLK_DIVIDER_ONE_BASED) {
		if (!val)
			val++;
	} else {
		val++;
	}

	return parent_rate / val;
}

struct clk_ops clk_divider_ops = {
	.set_rate = clk_divider_set_rate,
	.recalc_rate = clk_divider_recalc_rate,
};

struct clk *clk_divider(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	struct clk_divider *div = xzalloc(sizeof(*div));
	int ret;

	div->shift = shift;
	div->reg = reg;
	div->width = width;
	div->parent = parent;
	div->clk.ops = &clk_divider_ops;
	div->clk.name = name;
	div->clk.parent_names = &div->parent;
	div->clk.num_parents = 1;

	ret = clk_register(&div->clk);
	if (ret) {
		free(div);
		return ERR_PTR(ret);
	}

	return &div->clk;
}

struct clk *clk_divider_one_based(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	struct clk_divider *div;
	struct clk *clk;

	clk = clk_divider(name, parent, reg, shift, width);
	if (IS_ERR(clk))
		return clk;

	div = container_of(clk, struct clk_divider, clk);
	div->flags |= CLK_DIVIDER_ONE_BASED;

	return clk;
}

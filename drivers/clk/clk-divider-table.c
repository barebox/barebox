/*
 * clk-divider-table.c - generic barebox clock support. Based on Linux clk support
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

struct clk_divider_table {
	struct clk clk;
	u8 shift;
	u8 width;
	void __iomem *reg;
	const char *parent;
	const struct clk_div_table *table;
	int table_size;
	int max_div_index;
};

static int clk_divider_set_rate(struct clk *clk, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_divider_table *div =
		container_of(clk, struct clk_divider_table, clk);
	unsigned int val;
	int i, div_index = -1;
	unsigned long best = 0;

	if (rate > parent_rate)
		rate = parent_rate;
	if (rate < parent_rate / div->table[div->max_div_index].div)
		rate = parent_rate / div->table[div->max_div_index].div;

	for (i = 0; i < div->table_size; i++) {
		unsigned long now = parent_rate / div->table[i].div;

		if (now <= rate && now >= best) {
			best = now;
			div_index = i;
		}
	}

	val = readl(div->reg);
	val &= ~(((1 << div->width) - 1) << div->shift);
	val |= div_index << div->shift;
	writel(val, div->reg);

	return 0;
}

static unsigned long clk_divider_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	struct clk_divider_table *div =
		container_of(clk, struct clk_divider_table, clk);
	unsigned int val;

	val = readl(div->reg) >> div->shift;
	val &= (1 << div->width) - 1;

	if (val >= div->table_size)
		return 0;

	return parent_rate / div->table[val].div;
}

struct clk_ops clk_divider_table_ops = {
	.set_rate = clk_divider_set_rate,
	.recalc_rate = clk_divider_recalc_rate,
};

struct clk *clk_divider_table(const char *name,
		const char *parent, void __iomem *reg, u8 shift, u8 width,
		const struct clk_div_table *table)
{
	struct clk_divider_table *div = xzalloc(sizeof(*div));
	const struct clk_div_table *clkt;
	int ret, max_div = 0;

	div->shift = shift;
	div->reg = reg;
	div->width = width;
	div->parent = parent;
	div->clk.ops = &clk_divider_table_ops;
	div->clk.name = name;
	div->clk.parent_names = &div->parent;
	div->clk.num_parents = 1;
	div->table = table;

	for (clkt = div->table; clkt->div; clkt++) {
		if (clkt->div > max_div) {
			max_div = clkt->div;
			div->max_div_index = div->table_size;
		}
		div->table_size++;
	}

	ret = clk_register(&div->clk);
	if (ret) {
		free(div);
		return ERR_PTR(ret);
	}

	return &div->clk;
}

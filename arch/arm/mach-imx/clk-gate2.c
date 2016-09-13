/*
 * clk-gate2.c - barebox 2-bit clock support. Based on Linux clk support
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

#include "clk.h"


struct clk_gate2 {
	struct clk clk;
	void __iomem *reg;
	int shift;
	const char *parent;
#define CLK_GATE_INVERTED	(1 << 0)
	unsigned flags;
};

#define to_clk_gate2(_clk) container_of(_clk, struct clk_gate2, clk)

static int clk_gate2_enable(struct clk *clk)
{
	struct clk_gate2 *g = to_clk_gate2(clk);
	u32 val;

	val = readl(g->reg);

	if (g->flags & CLK_GATE_INVERTED)
		val &= ~(3 << g->shift);
	else
		val |= 3 << g->shift;

	writel(val, g->reg);

	return 0;
}

static void clk_gate2_disable(struct clk *clk)
{
	struct clk_gate2 *g = to_clk_gate2(clk);
	u32 val;

	val = readl(g->reg);

	if (g->flags & CLK_GATE_INVERTED)
		val |= 3 << g->shift;
	else
		val &= ~(3 << g->shift);

	writel(val, g->reg);
}

static int clk_gate2_is_enabled(struct clk *clk)
{
	struct clk_gate2 *g = to_clk_gate2(clk);
	u32 val;

	val = readl(g->reg);

	if (val & (1 << g->shift))
		return g->flags & CLK_GATE_INVERTED ? 0 : 1;
	else
		return g->flags & CLK_GATE_INVERTED ? 1 : 0;
}

static struct clk_ops clk_gate2_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.enable = clk_gate2_enable,
	.disable = clk_gate2_disable,
	.is_enabled = clk_gate2_is_enabled,
};

struct clk *clk_gate2_alloc(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	struct clk_gate2 *g = xzalloc(sizeof(*g));

	g->parent = parent;
	g->reg = reg;
	g->shift = shift;
	g->clk.ops = &clk_gate2_ops;
	g->clk.name = name;
	g->clk.parent_names = &g->parent;
	g->clk.num_parents = 1;
	g->clk.flags = CLK_SET_RATE_PARENT;

	return &g->clk;
}

void clk_gate2_free(struct clk *clk)
{
	struct clk_gate2 *g = to_clk_gate2(clk);

	free(g);
}

struct clk *clk_gate2(const char *name, const char *parent, void __iomem *reg,
		u8 shift)
{
	struct clk *g;
	int ret;

	g = clk_gate2_alloc(name , parent, reg, shift);

	ret = clk_register(g);
	if (ret) {
		free(to_clk_gate2(g));
		return ERR_PTR(ret);
	}

	return g;
}

struct clk *clk_gate2_inverted(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	struct clk *clk;
	struct clk_gate2 *g;

	clk = clk_gate2(name, parent, reg, shift);
	if (IS_ERR(clk))
		return clk;

	g = to_clk_gate2(clk);

	g->flags = CLK_GATE_INVERTED;

	return clk;
}

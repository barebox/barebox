/*
 * clk-gate.c - generic barebox clock support. Based on Linux clk support
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

struct clk_gate {
	struct clk clk;
	void __iomem *reg;
	int shift;
	const char *parent;
#define CLK_GATE_INVERTED	(1 << 0)
	unsigned flags;
};

#define to_clk_gate(_clk) container_of(_clk, struct clk_gate, clk)

static int clk_gate_enable(struct clk *clk)
{
	struct clk_gate *g = container_of(clk, struct clk_gate, clk);
	u32 val;

	val = readl(g->reg);

	if (g->flags & CLK_GATE_INVERTED)
		val &= ~(1 << g->shift);
	else
		val |= 1 << g->shift;

	writel(val, g->reg);

	return 0;
}

static void clk_gate_disable(struct clk *clk)
{
	struct clk_gate *g = container_of(clk, struct clk_gate, clk);
	u32 val;

	val = readl(g->reg);

	if (g->flags & CLK_GATE_INVERTED)
		val |= 1 << g->shift;
	else
		val &= ~(1 << g->shift);

	writel(val, g->reg);
}

static int clk_gate_is_enabled(struct clk *clk)
{
	struct clk_gate *g = container_of(clk, struct clk_gate, clk);
	u32 val;

	val = readl(g->reg);

	if (val & (1 << g->shift))
		return g->flags & CLK_GATE_INVERTED ? 0 : 1;
	else
		return g->flags & CLK_GATE_INVERTED ? 1 : 0;
}

struct clk_ops clk_gate_ops = {
	.enable = clk_gate_enable,
	.disable = clk_gate_disable,
	.is_enabled = clk_gate_is_enabled,
};

struct clk *clk_gate_alloc(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	struct clk_gate *g = xzalloc(sizeof(*g));

	g->parent = parent;
	g->reg = reg;
	g->shift = shift;
	g->clk.ops = &clk_gate_ops;
	g->clk.name = name;
	g->clk.parent_names = &g->parent;
	g->clk.num_parents = 1;

	return &g->clk;
}

void clk_gate_free(struct clk *clk_gate)
{
	struct clk_gate *g = to_clk_gate(clk_gate);

	free(g);
}

struct clk *clk_gate(const char *name, const char *parent, void __iomem *reg,
		u8 shift)
{
	struct clk *g;
	int ret;

	g = clk_gate_alloc(name , parent, reg, shift);

	ret = clk_register(g);
	if (ret) {
		free(to_clk_gate(g));
		return ERR_PTR(ret);
	}

	return g;
}

struct clk *clk_gate_inverted(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	struct clk *clk;
	struct clk_gate *g;

	clk = clk_gate(name, parent, reg, shift);
	if (IS_ERR(clk))
		return clk;

	g = container_of(clk, struct clk_gate, clk);

	g->flags = CLK_GATE_INVERTED;

	return clk;
}

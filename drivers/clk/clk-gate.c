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

static void clk_gate_endisable(struct clk *clk, int enable)
{
	struct clk_gate *gate = container_of(clk, struct clk_gate, clk);
	int set = gate->flags & CLK_GATE_INVERTED ? 1 : 0;
	u32 val;

	set ^= enable;

	if (gate->flags & CLK_GATE_HIWORD_MASK) {
		val = BIT(gate->shift + 16);
		if (set)
			val |= BIT(gate->shift);
	} else {
		val = readl(gate->reg);

		if (set)
			val |= BIT(gate->shift);
		else
			val &= ~BIT(gate->shift);
	}

	writel(val, gate->reg);
}

static int clk_gate_enable(struct clk *clk)
{
	clk_gate_endisable(clk, 1);

	return 0;
}

static void clk_gate_disable(struct clk *clk)
{
	clk_gate_endisable(clk, 0);
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
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.enable = clk_gate_enable,
	.disable = clk_gate_disable,
	.is_enabled = clk_gate_is_enabled,
};

struct clk *clk_gate_alloc(const char *name, const char *parent,
		void __iomem *reg, u8 shift, unsigned flags, u8 clk_gate_flags)
{
	struct clk_gate *g = xzalloc(sizeof(*g));

	g->parent = parent;
	g->reg = reg;
	g->shift = shift;
	g->clk.ops = &clk_gate_ops;
	g->clk.name = name;
	g->clk.flags = flags;
	g->clk.parent_names = &g->parent;
	g->clk.num_parents = 1;
	g->flags = clk_gate_flags;

	return &g->clk;
}

void clk_gate_free(struct clk *clk_gate)
{
	struct clk_gate *g = to_clk_gate(clk_gate);

	free(g);
}

struct clk *clk_gate(const char *name, const char *parent, void __iomem *reg,
		u8 shift, unsigned flags, u8 clk_gate_flags)
{
	struct clk *g;
	int ret;

	g = clk_gate_alloc(name , parent, reg, shift, flags, clk_gate_flags);

	ret = clk_register(g);
	if (ret) {
		free(to_clk_gate(g));
		return ERR_PTR(ret);
	}

	return g;
}

struct clk *clk_gate_inverted(const char *name, const char *parent,
		void __iomem *reg, u8 shift, unsigned flags)
{
	return clk_gate(name, parent, reg, shift, flags, CLK_GATE_INVERTED);
}

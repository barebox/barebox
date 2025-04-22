// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-gate.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

static void clk_gate_endisable(struct clk_hw *hw, int enable)
{
	struct clk_gate *gate = container_of(hw, struct clk_gate, hw);
	int set = gate->flags & CLK_GATE_SET_TO_DISABLE ? 1 : 0;
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

static int clk_gate_enable(struct clk_hw *hw)
{
	clk_gate_endisable(hw, 1);

	return 0;
}

static void clk_gate_disable(struct clk_hw *hw)
{
	clk_gate_endisable(hw, 0);
}

int clk_gate_is_enabled(struct clk_hw *hw)
{
	struct clk_gate *g = container_of(hw, struct clk_gate, hw);
	u32 val;

	val = readl(g->reg);

	if (val & (1 << g->shift))
		return g->flags & CLK_GATE_SET_TO_DISABLE ? 0 : 1;
	else
		return g->flags & CLK_GATE_SET_TO_DISABLE ? 1 : 0;
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

	g->_parent = parent;
	g->reg = reg;
	g->shift = shift;
	g->hw.clk.ops = &clk_gate_ops;
	g->hw.clk.name = name;
	g->hw.clk.flags = flags;
	g->hw.clk.parent_names = &g->_parent;
	g->hw.clk.num_parents = 1;
	g->flags = clk_gate_flags;

	return &g->hw.clk;
}

void clk_gate_free(struct clk *clk_gate)
{
	struct clk_hw *hw = clk_to_clk_hw(clk_gate);
	struct clk_gate *g = to_clk_gate(hw);

	free(g);
}

struct clk *clk_gate(const char *name, const char *parent, void __iomem *reg,
		u8 shift, unsigned flags, u8 clk_gate_flags)
{
	struct clk *g;
	int ret;

	g = clk_gate_alloc(name , parent, reg, shift, flags, clk_gate_flags);

	ret = bclk_register(g);
	if (ret) {
		struct clk_hw *hw = clk_to_clk_hw(g);
		free(to_clk_gate(hw));
		return ERR_PTR(ret);
	}

	return g;
}

struct clk *clk_gate_inverted(const char *name, const char *parent,
		void __iomem *reg, u8 shift, unsigned flags)
{
	return clk_gate(name, parent, reg, shift, flags, CLK_GATE_SET_TO_DISABLE);
}

struct clk *clk_register_gate(struct device *dev, const char *name,
			      const char *parent_name, unsigned long flags,
			      void __iomem *reg, u8 bit_idx,
			      u8 clk_gate_flags, spinlock_t *lock)
{
	return clk_gate(name, parent_name, reg, bit_idx, flags, clk_gate_flags);
}

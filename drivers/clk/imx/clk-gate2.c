// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-gate2.c - barebox 2-bit clock support. Based on Linux clk support
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clk.h"


struct clk_gate2 {
	struct clk_hw hw;
	void __iomem *reg;
	int shift;
	u8 cgr_val;
	const char *parent;
	unsigned flags;
};

static inline struct clk_gate2 *to_clk_gate2(struct clk_hw *hw)
{
	return container_of(hw, struct clk_gate2, hw);
}

static int clk_gate2_enable(struct clk_hw *hw)
{
	struct clk_gate2 *g = to_clk_gate2(hw);
	u32 val;

	val = readl(g->reg);

	if (g->flags & CLK_GATE_SET_TO_DISABLE)
		val &= ~(3 << g->shift);
	else
		val |= g->cgr_val << g->shift;

	writel(val, g->reg);

	return 0;
}

static void clk_gate2_disable(struct clk_hw *hw)
{
	struct clk_gate2 *g = to_clk_gate2(hw);
	u32 val;

	val = readl(g->reg);

	if (g->flags & CLK_GATE_SET_TO_DISABLE)
		val |= 3 << g->shift;
	else
		val &= ~(3 << g->shift);

	writel(val, g->reg);
}

static int clk_gate2_is_enabled(struct clk_hw *hw)
{
	struct clk_gate2 *g = to_clk_gate2(hw);
	u32 val;

	val = readl(g->reg);

	if (val & (1 << g->shift))
		return g->flags & CLK_GATE_SET_TO_DISABLE ? 0 : 1;
	else
		return g->flags & CLK_GATE_SET_TO_DISABLE ? 1 : 0;
}

static struct clk_ops clk_gate2_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.enable = clk_gate2_enable,
	.disable = clk_gate2_disable,
	.is_enabled = clk_gate2_is_enabled,
};

static struct clk *clk_gate2_alloc(const char *name, const char *parent,
			    void __iomem *reg, u8 shift, u8 cgr_val,
			    unsigned long flags)
{
	struct clk_gate2 *g = xzalloc(sizeof(*g));

	g->parent = parent;
	g->reg = reg;
	g->cgr_val = cgr_val;
	g->shift = shift;
	g->hw.clk.ops = &clk_gate2_ops;
	g->hw.clk.name = name;
	g->hw.clk.parent_names = &g->parent;
	g->hw.clk.num_parents = 1;
	g->hw.clk.flags = CLK_SET_RATE_PARENT | flags;

	return &g->hw.clk;
}

struct clk *clk_gate2(const char *name, const char *parent, void __iomem *reg,
		      u8 shift, u8 cgr_val, unsigned long flags)
{
	struct clk *g;
	int ret;

	g = clk_gate2_alloc(name , parent, reg, shift, cgr_val, flags);

	ret = bclk_register(g);
	if (ret) {
		struct clk_hw *hw = clk_to_clk_hw(g);
		free(to_clk_gate2(hw));
		return ERR_PTR(ret);
	}

	return g;
}

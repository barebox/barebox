// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-mux.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

static int clk_mux_get_parent(struct clk_hw *hw)
{
	struct clk_mux *m = to_clk_mux(hw);
	int idx = readl(m->reg) >> m->shift & ((1 << m->width) - 1);

	return idx;
}

static int clk_mux_set_parent(struct clk_hw *hw, u8 idx)
{
	struct clk_mux *m = to_clk_mux(hw);
	u32 val;

	if (m->flags & CLK_MUX_READ_ONLY) {
		if (clk_mux_get_parent(hw) != idx)
			return -EPERM;
		else
			return 0;
	}

	val = readl(m->reg);
	val &= ~(((1 << m->width) - 1) << m->shift);
	val |= idx << m->shift;

	if (m->flags & CLK_MUX_HIWORD_MASK)
		val |= ((1 << m->width) - 1) << (m->shift + 16);
	writel(val, m->reg);

	return 0;
}

struct clk_ops clk_mux_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.get_parent = clk_mux_get_parent,
	.set_parent = clk_mux_set_parent,
};

struct clk *clk_mux_alloc(const char *name, unsigned clk_flags, void __iomem *reg,
		u8 shift, u8 width, const char * const *parents, u8 num_parents,
		unsigned mux_flags)
{
	struct clk_mux *m = xzalloc(sizeof(*m));

	m->reg = reg;
	m->shift = shift;
	m->width = width;
	m->flags = mux_flags;
	m->hw.clk.ops = &clk_mux_ops;
	m->hw.clk.name = name;
	m->hw.clk.flags = clk_flags;
	m->hw.clk.parent_names = parents;
	m->hw.clk.num_parents = num_parents;

	return &m->hw.clk;
}

void clk_mux_free(struct clk *clk_mux)
{
	struct clk_hw *hw = clk_to_clk_hw(clk_mux);
	struct clk_mux *m = to_clk_mux(hw);

	free(m);
}

struct clk *clk_mux(const char *name, unsigned clk_flags, void __iomem *reg,
		    u8 shift, u8 width, const char * const *parents,
		    u8 num_parents, unsigned mux_flags)
{
	struct clk *m;
	int ret;

	m = clk_mux_alloc(name, clk_flags, reg, shift, width, parents,
			  num_parents, mux_flags);

	ret = bclk_register(m);
	if (ret) {
		struct clk_hw *hw = clk_to_clk_hw(m);
		free(to_clk_mux(hw));
		return ERR_PTR(ret);
	}

	return m;
}

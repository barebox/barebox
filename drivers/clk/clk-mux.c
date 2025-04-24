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

int clk_mux_val_to_index(struct clk_hw *hw, u32 *table, unsigned int flags,
			 unsigned int val)
{
	int num_parents = clk_hw_get_num_parents(hw);

	if (table) {
		int i;

		for (i = 0; i < num_parents; i++)
			if (table[i] == val)
				return i;
		return -EINVAL;
	}

	return val;
}
EXPORT_SYMBOL_GPL(clk_mux_val_to_index);

unsigned int clk_mux_index_to_val(u32 *table, unsigned int flags, u8 index)
{
	return table ? table[index] : index;
}
EXPORT_SYMBOL_GPL(clk_mux_index_to_val);

static int clk_mux_get_parent(struct clk_hw *hw)
{
	struct clk_mux *m = to_clk_mux(hw);
	u32 idx;

	idx = readl(m->reg) >> m->shift;
	idx &= m->mask;

	return clk_mux_val_to_index(hw, m->table, m->flags, idx);
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

	idx = clk_mux_index_to_val(m->table, m->flags, idx);

	val = readl(m->reg);
	val &= ~(m->mask << m->shift);
	val |= idx << m->shift;

	if (m->flags & CLK_MUX_HIWORD_MASK)
		val |= m->mask << (m->shift + 16);
	writel(val, m->reg);

	return 0;
}

static struct clk *clk_get_parent_index(struct clk *clk, int num)
{
	if (num >= clk->num_parents)
		return NULL;

	if (clk->parents[num])
		return clk->parents[num];

	clk->parents[num] = clk_lookup(clk->parent_names[num]);

	return clk->parents[num];
}

static struct clk *clk_mux_best_parent(struct clk *mux, unsigned long rate,
	unsigned long *rrate)
{
	struct clk *bestparent = NULL;
	long bestrate = LONG_MAX;
	int i;

	for (i = 0; i < mux->num_parents; i++) {
		struct clk *parent = clk_get_parent_index(mux, i);
		unsigned long r;

		if (IS_ERR_OR_NULL(parent))
			continue;

		if (mux->flags & CLK_SET_RATE_PARENT)
			r = clk_round_rate(parent, rate);
		else
			r = clk_get_rate(parent);

		if (abs((long)rate - r) < abs((long)rate - bestrate)) {
			bestrate = r;
			bestparent = parent;
		}
	}

	*rrate = bestrate;

	return bestparent;
}

long clk_mux_round_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long *prate)
{
	struct clk *clk = clk_hw_to_clk(hw);
	unsigned long rrate;
	struct clk *bestparent;

	if (clk->flags & CLK_SET_RATE_NO_REPARENT)
		return clk_parent_round_rate(hw, rate, prate);

	bestparent = clk_mux_best_parent(clk, rate, &rrate);

	return rrate;
}
EXPORT_SYMBOL_GPL(clk_mux_round_rate);

static int clk_mux_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct clk *clk = clk_hw_to_clk(hw);
	struct clk *parent;
	unsigned long rrate;
	int ret;

	if (clk->flags & CLK_SET_RATE_NO_REPARENT)
		return clk_parent_set_rate(hw, rate, parent_rate);

	parent = clk_mux_best_parent(clk, rate, &rrate);

	ret = clk_set_parent(clk, parent);
	if (ret)
		return ret;

	return clk_set_rate(parent, rate);
}

const struct clk_ops clk_mux_ops = {
	.set_rate = clk_mux_set_rate,
	.round_rate = clk_mux_round_rate,
	.get_parent = clk_mux_get_parent,
	.set_parent = clk_mux_set_parent,
};

const struct clk_ops clk_mux_ro_ops = {
	.get_parent = clk_mux_get_parent,
};

struct clk *clk_mux_alloc(const char *name, unsigned clk_flags, void __iomem *reg,
		u8 shift, u8 width, const char * const *parents, u8 num_parents,
		unsigned mux_flags)
{
	struct clk_mux *m = xzalloc(sizeof(*m));

	m->reg = reg;
	m->shift = shift;
	m->mask = (1 << width) - 1;
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

struct clk *clk_register_mux(struct device *dev, const char *name,
			     const char * const *parent_names, u8 num_parents,
			     unsigned long flags,
			     void __iomem *reg, u8 shift, u8 width,
			     u8 clk_mux_flags, spinlock_t *lock)
{
	return clk_mux(name, flags, reg, shift, width, parent_names,
		       num_parents, clk_mux_flags);
}

struct clk_hw *__clk_hw_register_mux(struct device *dev,
				     const char *name, u8 num_parents,
				     const char * const *parent_names,
				     unsigned long flags, void __iomem *reg,
				     u8 shift, u32 mask,
				     u8 clk_mux_flags, u32 *table,
				     spinlock_t *lock)
{
	struct clk_mux *mux;
	struct clk_hw *hw;
	struct clk_init_data init = {};
	int ret = -EINVAL;

	if (clk_mux_flags & CLK_MUX_HIWORD_MASK) {
		u8 width = fls(mask) - ffs(mask) + 1;

		if (width + shift > 16) {
			pr_err("mux value exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the mux */
	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	if (clk_mux_flags & CLK_MUX_READ_ONLY)
		init.ops = &clk_mux_ro_ops;
	else
		init.ops = &clk_mux_ops;
	init.flags = flags;
	init.parent_names = parent_names;
	init.num_parents = num_parents;

	/* struct clk_mux assignments */
	mux->reg = reg;
	mux->shift = shift;
	mux->mask = mask;
	mux->flags = clk_mux_flags;
	mux->lock = lock;
	mux->table = table;
	mux->hw.init = &init;

	hw = &mux->hw;
	ret = clk_hw_register(dev, hw);
	if (ret) {
		kfree(mux);
		hw = ERR_PTR(ret);
	}

	return hw;
}

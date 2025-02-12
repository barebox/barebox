// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Jules Maselbas
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include "clk-sunxi.h"

/* clk_hw to ccu_clk */
#define to_ccu_clk(_hw) container_of(_hw, struct ccu_clk, hw)

static void ccu_clk_gate(struct ccu_clk *clk, int enable)
{
	u32 val, old;

	if (!clk->gate)
		return;

	old = val = readl(clk->base + clk->reg);
	if (enable)
		val |= clk->gate;
	else
		val &= ~clk->gate;
	writel(val, clk->base + clk->reg);
}

static int ccu_clk_enable(struct clk_hw *hw)
{
	struct ccu_clk *clk = to_ccu_clk(hw);

	ccu_clk_gate(clk, 1);

	return 0;
}

static void ccu_clk_disable(struct clk_hw *hw)
{
	struct ccu_clk *clk = to_ccu_clk(hw);

	ccu_clk_gate(clk, 0);
}

static int ccu_clk_is_enabled(struct clk_hw *hw)
{
	struct ccu_clk *clk = to_ccu_clk(hw);

	if (clk->gate)
		return readl(clk->base + clk->reg) & clk->gate;

	return 1;
}

static int ccu_clk_get_parent(struct clk_hw *hw)
{
	struct ccu_clk *clk = to_ccu_clk(hw);
	u32 reg, val;

	if (!clk->mux.width)
		return 0;

	reg = readl(clk->base + clk->reg);
	val = reg >> clk->mux.shift;
	val &= (1 << clk->mux.width) - 1;

	return clk_mux_val_to_index(hw, (u32 *)clk->mux.table, 0, val);
}

static int ccu_clk_set_parent(struct clk_hw *hw, u8 index)
{
	struct ccu_clk *clk = to_ccu_clk(hw);
	u32 reg, val;

	if (!clk->mux.width)
		return 0;

	/* index to val */
	val = clk->mux.table ? clk->mux.table[index] : index;

	reg = readl(clk->base + clk->reg);
	reg &= ~(((1 << clk->mux.width) - 1) << clk->mux.shift);
	reg |= val << clk->mux.shift;

	writel(reg, clk->base + clk->reg);

	return 0;
}

static u32 ccu_clk_get_div(struct ccu_clk_div *div, u32 val)
{
	u32 d;

	switch (div->type) {
	case CCU_CLK_DIV_FIXED:
		d = div->off;
		break;
	case CCU_CLK_DIV_M:
		d = val + 1 + div->off;
		break;
	case CCU_CLK_DIV_P:
		d = (1 << val) + div->off;
		break;
	default:
		d = 1;
		break;
	}

	return d;
}

static u32 ccu_clk_min_div(struct ccu_clk *clk)
{
	u32 d = 1;
	size_t i;

	for (i = 0; i < 4; i++)
		d *= ccu_clk_get_div(&clk->div[i], 0);

	return d;
}

static u32 ccu_clk_max_div(struct ccu_clk *clk)
{
	u32 d = 1;
	size_t i;

	for (i = 0; i < 4; i++)
		d *= ccu_clk_get_div(&clk->div[i], (1 << clk->div[i].width) - 1);

	return d;
}

static unsigned long ccu_clk_find_best_div(struct ccu_clk *clk,
				       unsigned long rate, unsigned long prate,
				       u32 best_div[4])
{
	unsigned long r, best_rate = 0;
	u32 i0, i1, i2, i3;
	u32 m0, m1, m2, m3;
	u32 d0, d1, d2, d3;

	m0 = (1 << clk->div[0].width);
	m1 = (1 << clk->div[1].width);
	m2 = (1 << clk->div[2].width);
	m3 = (1 << clk->div[3].width);

	for (i3 = 0; i3 < m3; i3++) {
		d3 = ccu_clk_get_div(&clk->div[3], i3);
		for (i2 = 0; i2 < m2; i2++) {
			d2 = d3 * ccu_clk_get_div(&clk->div[2], i2);
			for (i1 = 0; i1 < m1; i1++) {
				d1 = d2 * ccu_clk_get_div(&clk->div[1], i1);
				for (i0 = 0; i0  < m0; i0++) {
					d0 = d1 * ccu_clk_get_div(&clk->div[0], i0);
					r = prate / d0;
					if (r > rate)
						continue;
					if ((rate - r) < (rate - best_rate)) {
						best_rate = r;
						if (best_div) {
							best_div[0] = i0;
							best_div[1] = i1;
							best_div[2] = i2;
							best_div[3] = i3;
						}
					}
				}
			}
		}
	}

	return best_rate;
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

static unsigned long ccu_clk_find_best_parent(struct ccu_clk *clk, unsigned long rate, int *idx)
{
	u32 min_div, max_div;
	unsigned long min_rate, max_rate;
	size_t i;

	min_div = ccu_clk_min_div(clk);
	max_div = ccu_clk_max_div(clk);

	min_rate = rate * min_div;
	max_rate = rate * max_div;

	for (i = 0; i < clk->hw.clk.num_parents; i++) {
		struct clk *parent = clk_get_parent_index(&clk->hw.clk, i);
		unsigned long pr;

		if (IS_ERR_OR_NULL(parent))
			continue;

		pr = clk_get_rate(parent);
		if (min_rate <= pr && pr <= max_rate) {
			if (idx)
				*idx = i;
			return pr;
		}
	}

	return 0;
}

static int ccu_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			    unsigned long prate)
{
	struct ccu_clk *clk = to_ccu_clk(hw);
	unsigned long rrate;
	size_t i;
	u32 dval[4] = {};
	u32 reg, val, msk;
	int idx;

	prate = ccu_clk_find_best_parent(clk, rate, &idx);
	ccu_clk_set_parent(hw, idx);

	rrate = ccu_clk_find_best_div(clk, rate, prate, dval);
	val = 0;
	msk = 0;
	for (i = 0; i < 4; i++) {
		val |= dval[i] << clk->div[i].shift;
		msk |= ((1 << clk->div[i].width) - 1) << clk->div[i].shift;
	}

	reg = readl(clk->base + clk->reg);
	reg &= ~msk;
	reg |= val & msk;
	writel(reg, clk->base + clk->reg);

	return 0;
}

static long ccu_clk_round_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long *prate)
{
	struct ccu_clk *clk = to_ccu_clk(hw);

	if (clk->mux.width)
		*prate = ccu_clk_find_best_parent(clk, rate, NULL);

	return ccu_clk_find_best_div(clk, rate, *prate, NULL);
}

static unsigned long ccu_clk_recalc_rate(struct clk_hw *hw,
					unsigned long prate)
{
	struct ccu_clk *clk = to_ccu_clk(hw);
	unsigned long rate = prate;
	size_t i;
	u32 reg, val, div;

	div = 1;
	reg = readl(clk->base + clk->reg);
	for (i = 0; i < clk->num_div; i++) {
		val = reg >> clk->div[i].shift;
		val &= (1 << clk->div[i].width) - 1;
		div *= ccu_clk_get_div(&clk->div[i], val);
	}
	rate = prate / div;

	return rate;
}

const struct clk_ops ccu_clk_ops = {
	/* gate */
	.disable        = ccu_clk_disable,
	.enable         = ccu_clk_enable,
	.is_enabled     = ccu_clk_is_enabled,
	/* mux */
	.get_parent	= ccu_clk_get_parent,
	.set_parent	= ccu_clk_set_parent,
	/* rate */
	.round_rate     = ccu_clk_round_rate,
	.set_rate       = ccu_clk_set_rate,
	.recalc_rate    = ccu_clk_recalc_rate,
};

static struct clk *sunxi_ccu_clk_alloc(const char *name,
				   const char * const *parents, u8 num_parents,
				   void __iomem *base, u32 reg,
				   struct ccu_clk_mux mux,
				   const struct ccu_clk_div *div, u8 num_div,
				   u32 gate,
				   int flags)
{
	struct ccu_clk *clk;
	size_t i;

	clk = xzalloc(sizeof(*clk));
	clk->base = base;
	clk->reg = reg;

	clk->mux = mux;
	clk->num_div = num_div > 4 ? 4 : num_div;
	for (i = 0; i < clk->num_div; i++)
		clk->div[i] = div[i];
	clk->gate = gate;

	clk->hw.clk.ops = &ccu_clk_ops;
	clk->hw.clk.name = name;
	clk->hw.clk.flags = flags;
	clk->hw.clk.parent_names = parents;
	clk->hw.clk.num_parents = num_parents;

	return &clk->hw.clk;
}

struct clk *sunxi_ccu_clk(const char *name,
			  const char * const *parents, u8 num_parents,
			  void __iomem *base, u32 reg,
			  struct ccu_clk_mux mux,
			  const struct ccu_clk_div *div, u8 num_div,
			  u32 gate,
			  int flags)
{
	struct clk *c;
	int ret;

	c = sunxi_ccu_clk_alloc(name, parents, num_parents,
				base, reg,
				mux,
				div, num_div,
				gate,
				flags);

	ret = bclk_register(c);
	if (ret) {
		struct clk_hw *hw = clk_to_clk_hw(c);
		free(to_ccu_clk(hw));
		return ERR_PTR(ret);
	}

	return c;
}

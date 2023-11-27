// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-divider.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/log2.h>
#include <linux/math64.h>

static unsigned int _get_table_maxdiv(const struct clk_div_table *table)
{
	unsigned int maxdiv = 0;
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div > maxdiv)
			maxdiv = clkt->div;
	return maxdiv;
}

static unsigned int _get_maxdiv(const struct clk_div_table *table, u8 width,
				unsigned long flags)
{
	if (flags & CLK_DIVIDER_ONE_BASED)
		return clk_div_mask(width);
	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << clk_div_mask(width);
	if (table)
		return _get_table_maxdiv(table);
	return clk_div_mask(width) + 1;
}

static unsigned int _get_table_div(const struct clk_div_table *table,
							unsigned int val)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->val == val)
			return clkt->div;
	return 0;
}

static unsigned int _get_div(const struct clk_div_table *table,
			     unsigned int val, unsigned long flags, u8 width)
{
	if (flags & CLK_DIVIDER_ONE_BASED)
		return val;
	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << val;
	if (table)
		return _get_table_div(table, val);
	return val + 1;
}
static unsigned int _get_table_val(const struct clk_div_table *table,
							unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return clkt->val;
	return 0;
}

static unsigned int _get_val(const struct clk_div_table *table,
			     unsigned int div, unsigned long flags)
{
	if (flags & CLK_DIVIDER_ONE_BASED)
		return div;
	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		return __ffs(div);
	if (table)
		return  _get_table_val(table, div);
	return div - 1;
}

unsigned long divider_recalc_rate(struct clk *clk, unsigned long parent_rate,
		unsigned int val,
		const struct clk_div_table *table,
		unsigned long flags, unsigned long width)
{
	unsigned int div;

	div = _get_div(table, val, flags, width);
	if (!div) {
		WARN(!(flags & CLK_DIVIDER_ALLOW_ZERO),
			"%s: Zero divisor and CLK_DIVIDER_ALLOW_ZERO not set\n",
			clk->name);
		return parent_rate;
	}

	return DIV_ROUND_UP_ULL((u64)parent_rate, div);
}

static unsigned long clk_divider_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk *clk = clk_hw_to_clk(hw);
	struct clk_divider *divider = container_of(hw, struct clk_divider, hw);
	unsigned int val;

	val = readl(divider->reg) >> divider->shift;
	val &= clk_div_mask(divider->width);

	return divider_recalc_rate(clk, parent_rate, val, divider->table,
				   divider->flags, divider->width);
}

/*
 * The reverse of DIV_ROUND_UP: The maximum number which
 * divided by m is r
 */
#define MULT_ROUND_UP(r, m) ((r) * (m) + (m) - 1)

static bool _is_valid_table_div(const struct clk_div_table *table,
							 unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return true;
	return false;
}

static bool _is_valid_div(const struct clk_div_table *table, unsigned int div,
			  unsigned long flags)
{
	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		return is_power_of_2(div);
	if (table)
		return _is_valid_table_div(table, div);
	return true;
}

static int _round_up_table(const struct clk_div_table *table, int div)
{
	const struct clk_div_table *clkt;
	int up = _get_table_maxdiv(table);

	for (clkt = table; clkt->div; clkt++) {
		if (clkt->div == div)
			return clkt->div;
		else if (clkt->div < div)
			continue;

		if ((clkt->div - div) < (up - div))
			up = clkt->div;
	}

	return up;
}

static int _div_round_up(const struct clk_div_table *table,
			 unsigned long parent_rate, unsigned long rate,
			 unsigned long flags)
{
	int div = DIV_ROUND_UP(parent_rate, rate);

	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		div = __roundup_pow_of_two(div);
	if (table)
		div = _round_up_table(table, div);

	return div;
}

static int clk_divider_bestdiv(struct clk *clk, unsigned long rate,
			       unsigned long *best_parent_rate,
			       const struct clk_div_table *table, u8 width,
			       unsigned long flags)
{
	int i, bestdiv = 0;
	unsigned long parent_rate, best = 0, now, maxdiv;
	unsigned long parent_rate_saved = *best_parent_rate;

	if (!rate)
		rate = 1;

	maxdiv = _get_maxdiv(table, width, flags);

	if (!(clk->flags & CLK_SET_RATE_PARENT)) {
		parent_rate = *best_parent_rate;
		bestdiv = _div_round_up(table, parent_rate, rate, flags);
		bestdiv = bestdiv == 0 ? 1 : bestdiv;
		bestdiv = bestdiv > maxdiv ? maxdiv : bestdiv;
		return bestdiv;
	}

	/*
	 * The maximum divider we can use without overflowing
	 * unsigned long in rate * i below
	 */
	maxdiv = min(ULONG_MAX / rate, maxdiv);

	for (i = 1; i <= maxdiv; i++) {
		if (!_is_valid_div(table, i, flags))
			continue;
		if (rate * i == parent_rate_saved) {
			/*
			 * It's the most ideal case if the requested rate can be
			 * divided from parent clock without needing to change
			 * parent rate, so return the divider immediately.
			 */
			*best_parent_rate = parent_rate_saved;
			return i;
		}
		parent_rate = clk_round_rate(clk_get_parent(clk),
				MULT_ROUND_UP(rate, i));
		now = parent_rate / i;
		if (now <= rate && now > best) {
			bestdiv = i;
			best = now;
			*best_parent_rate = parent_rate;
		}
	}

	if (!bestdiv) {
		bestdiv = _get_maxdiv(table, width, flags);
		*best_parent_rate = clk_round_rate(clk_get_parent(clk), 1);
	}

	return bestdiv;
}

long divider_round_rate(struct clk *clk, unsigned long rate,
			unsigned long *prate, const struct clk_div_table *table,
			u8 width, unsigned long flags)
{
	int div;

	div = clk_divider_bestdiv(clk, rate, prate, table, width, flags);

	return DIV_ROUND_UP(*prate, div);
}

static long clk_divider_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	struct clk *clk = clk_hw_to_clk(hw);
	struct clk_divider *divider = to_clk_divider(hw);

	if (divider->flags & CLK_DIVIDER_READ_ONLY)
		return clk_divider_recalc_rate(hw, *prate);

	return divider_round_rate(clk, rate, prate, divider->table,
				  divider->width, divider->flags);
}

int divider_get_val(unsigned long rate, unsigned long parent_rate,
		    const struct clk_div_table *table, u8 width,
		    unsigned long flags)
{
	unsigned int div, value;

	div = DIV_ROUND_UP(parent_rate, rate);

	if (!_is_valid_div(table, div, flags))
		return -EINVAL;

	value = _get_val(table, div, flags);

	return min_t(unsigned int, value, clk_div_mask(width));
}

static int clk_divider_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk *clk = clk_hw_to_clk(hw);
	struct clk_divider *divider = to_clk_divider(hw);
	unsigned int value;
	u32 val;

	if (divider->flags & CLK_DIVIDER_READ_ONLY)
		return 0;

	if (clk->flags & CLK_SET_RATE_PARENT) {
		clk_divider_bestdiv(clk, rate, &parent_rate,
				    divider->table, divider->width,
				    divider->flags);
		clk_set_rate(clk_get_parent(clk), parent_rate);
	}

	value = divider_get_val(rate, parent_rate, divider->table,
				divider->width, divider->flags);

	val = readl(divider->reg);
	val &= ~(clk_div_mask(divider->width) << divider->shift);
	val |= value << divider->shift;

	if (divider->flags & CLK_DIVIDER_HIWORD_MASK)
		val |= clk_div_mask(divider->width) << (divider->shift + 16);

	writel(val, divider->reg);

	return 0;
}

const struct clk_ops clk_divider_ops = {
	.set_rate = clk_divider_set_rate,
	.recalc_rate = clk_divider_recalc_rate,
	.round_rate = clk_divider_round_rate,
};

const struct clk_ops clk_divider_ro_ops = {
	.recalc_rate = clk_divider_recalc_rate,
};

struct clk *clk_divider_alloc(const char *name, const char *parent,
			      unsigned clk_flags, void __iomem *reg, u8 shift,
			      u8 width, unsigned div_flags)
{
	struct clk_divider *div = xzalloc(sizeof(*div));

	div->shift = shift;
	div->reg = reg;
	div->width = width;
	div->parent = parent;
	div->flags = div_flags;
	div->hw.clk.ops = &clk_divider_ops;
	div->hw.clk.name = name;
	div->hw.clk.flags = clk_flags;
	div->hw.clk.parent_names = &div->parent;
	div->hw.clk.num_parents = 1;

	return &div->hw.clk;
}

void clk_divider_free(struct clk *clk)
{
	struct clk_hw *hw = clk_to_clk_hw(clk);
	struct clk_divider *d = to_clk_divider(hw);

	free(d);
}

struct clk *clk_divider(const char *name, const char *parent, unsigned clk_flags,
			void __iomem *reg, u8 shift, u8 width, unsigned div_flags)
{
	struct clk *d;
	int ret;

	d = clk_divider_alloc(name , parent, clk_flags, reg, shift, width,
			      div_flags);

	ret = bclk_register(d);
	if (ret) {
		clk_divider_free(d);
		return ERR_PTR(ret);
	}

	return d;
}

struct clk *clk_divider_one_based(const char *name, const char *parent,
				  unsigned clk_flags, void __iomem *reg, u8 shift,
				  u8 width, unsigned div_flags)
{
	struct clk_divider *div;
	struct clk *clk;
	struct clk_hw *hw;

	clk = clk_divider(name, parent, clk_flags, reg, shift, width, div_flags);
	if (IS_ERR(clk))
		return clk;

	hw = clk_to_clk_hw(clk);
	div = to_clk_divider(hw);

	div->flags |= CLK_DIVIDER_ONE_BASED;

	return clk;
}

struct clk *clk_divider_table(const char *name, const char *parent,
			      unsigned clk_flags, void __iomem *reg, u8 shift,
			      u8 width, const struct clk_div_table *table,
			      unsigned div_flags)
{
	struct clk_divider *div = xzalloc(sizeof(*div));
	const struct clk_div_table *clkt;
	int ret, max_div = 0;

	div->shift = shift;
	div->reg = reg;
	div->width = width;
	div->parent = parent;
	div->flags = div_flags;
	div->hw.clk.ops = &clk_divider_ops;
	div->hw.clk.name = name;
	div->hw.clk.flags = clk_flags;
	div->hw.clk.parent_names = &div->parent;
	div->hw.clk.num_parents = 1;
	div->table = table;

	for (clkt = div->table; clkt->div; clkt++) {
		if (clkt->div > max_div) {
			max_div = clkt->div;
			div->max_div_index = div->table_size;
		}
		div->table_size++;
	}

	ret = bclk_register(&div->hw.clk);
	if (ret) {
		free(div);
		return ERR_PTR(ret);
	}

	return &div->hw.clk;
}

struct clk *clk_register_divider_table(struct device *dev, const char *name,
				       const char *parent_name,
				       unsigned long flags,
				       void __iomem *reg, u8 shift, u8 width,
				       u8 clk_divider_flags,
				       const struct clk_div_table *table,
				       spinlock_t *lock)
{
	return clk_divider_table(name, parent_name, flags, reg, shift, width,
				 table, clk_divider_flags);
}

struct clk *clk_register_divider(struct device *dev, const char *name,
				 const char *parent_name, unsigned long flags,
				 void __iomem *reg, u8 shift, u8 width,
				 u8 clk_divider_flags, spinlock_t *lock)
{
	return clk_divider(name, parent_name, flags, reg, shift, width,
			   clk_divider_flags);
}

struct clk_hw *clk_hw_register_divider_table(struct device *dev,
					     const char *name,
					     const char *parent_name,
					     unsigned long flags,
					     void __iomem *reg, u8 shift,
					     u8 width,
					     u8 clk_divider_flags,
					     const struct clk_div_table *table,
					     spinlock_t *lock)
{
	return clk_to_clk_hw(clk_register_divider_table(dev, xstrdup(name),
		xstrdup(parent_name), flags, reg, shift, width,
		clk_divider_flags, table, lock));
}

struct clk_hw *clk_hw_register_divider(struct device *dev,
				       const char *name,
				       const char *parent_name,
				       unsigned long flags,
				       void __iomem *reg, u8 shift, u8 width,
				       u8 clk_divider_flags, spinlock_t *lock)
{
	return clk_to_clk_hw(clk_register_divider(dev, xstrdup(name),
		xstrdup(parent_name), flags, reg, shift, width,
		clk_divider_flags, lock));
}

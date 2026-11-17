// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-divider.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/log2.h>
#include <linux/math64.h>

static unsigned int _get_table_maxdiv(const struct clk_div_table *table,
				      u8 width)
{
	unsigned int maxdiv = 0, mask = clk_div_mask(width);
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div > maxdiv && clkt->val <= mask)
			maxdiv = clkt->div;
	return maxdiv;
}

static unsigned int _get_table_mindiv(const struct clk_div_table *table)
{
	unsigned int mindiv = UINT_MAX;
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div < mindiv)
			mindiv = clkt->div;
	return mindiv;
}

static unsigned int _get_maxdiv(const struct clk_div_table *table, u8 width,
				unsigned long flags)
{
	if (flags & CLK_DIVIDER_ONE_BASED)
		return clk_div_mask(width);
	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << clk_div_mask(width);
	if (table)
		return _get_table_maxdiv(table, width);
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
			"%pC: Zero divisor and CLK_DIVIDER_ALLOW_ZERO not set\n",
			clk);
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
	int up = INT_MAX;

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

static int _round_down_table(const struct clk_div_table *table, int div)
{
	const struct clk_div_table *clkt;
	int down = _get_table_mindiv(table);

	for (clkt = table; clkt->div; clkt++) {
		if (clkt->div == div)
			return clkt->div;
		else if (clkt->div > div)
			continue;

		if ((div - clkt->div) < (div - down))
			down = clkt->div;
	}

	return down;
}

static int _div_round_up(const struct clk_div_table *table,
			 unsigned long parent_rate, unsigned long rate,
			 unsigned long flags)
{
	int div = DIV_ROUND_UP_ULL((u64)parent_rate, rate);

	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		div = __roundup_pow_of_two(div);
	if (table)
		div = _round_up_table(table, div);

	return div;
}

static int _div_round_closest(const struct clk_div_table *table,
			      unsigned long parent_rate, unsigned long rate,
			      unsigned long flags)
{
	int up, down;
	unsigned long up_rate, down_rate;

	up = DIV_ROUND_UP_ULL((u64)parent_rate, rate);
	down = parent_rate / rate;

	/*
	 * Unlike Linux, clamp the round-down candidate: a request above the
	 * parent rate leaves it at zero and both __rounddown_pow_of_two() and
	 * the DIV_ROUND_UP_ULL() below are undefined for a zero divisor.
	 */
	if (!down)
		down = 1;

	if (flags & CLK_DIVIDER_POWER_OF_TWO) {
		up = __roundup_pow_of_two(up);
		down = __rounddown_pow_of_two(down);
	} else if (table) {
		up = _round_up_table(table, up);
		down = _round_down_table(table, down);
	}

	up_rate = DIV_ROUND_UP_ULL((u64)parent_rate, up);
	down_rate = DIV_ROUND_UP_ULL((u64)parent_rate, down);

	return (rate - up_rate) <= (down_rate - rate) ? up : down;
}

static int _div_round(const struct clk_div_table *table,
		      unsigned long parent_rate, unsigned long rate,
		      unsigned long flags)
{
	if (flags & CLK_DIVIDER_ROUND_CLOSEST)
		return _div_round_closest(table, parent_rate, rate, flags);

	return _div_round_up(table, parent_rate, rate, flags);
}

static bool _is_best_div(unsigned long rate, unsigned long now,
			 unsigned long best, unsigned long flags)
{
	if (flags & CLK_DIVIDER_ROUND_CLOSEST)
		return abs(rate - now) < abs(rate - best);

	return now <= rate && now > best;
}

static int clk_divider_bestdiv(struct clk_hw *hw, struct clk_hw *parent,
			       unsigned long rate,
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

	if (!(clk_hw_get_flags(hw) & CLK_SET_RATE_PARENT)) {
		parent_rate = *best_parent_rate;
		bestdiv = _div_round(table, parent_rate, rate, flags);
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
		parent_rate = clk_hw_round_rate(parent, rate * i);
		now = DIV_ROUND_UP_ULL((u64)parent_rate, i);
		if (_is_best_div(rate, now, best, flags)) {
			bestdiv = i;
			best = now;
			*best_parent_rate = parent_rate;
		}
	}

	if (!bestdiv) {
		bestdiv = _get_maxdiv(table, width, flags);
		*best_parent_rate = clk_hw_round_rate(parent, 1);
	}

	return bestdiv;
}

int divider_determine_rate(struct clk_hw *hw, struct clk_rate_request *req,
			   const struct clk_div_table *table, u8 width,
			   unsigned long flags)
{
	int div;

	div = clk_divider_bestdiv(hw, req->best_parent_hw, req->rate,
				  &req->best_parent_rate, table, width, flags);

	req->rate = DIV_ROUND_UP_ULL((u64)req->best_parent_rate, div);

	return 0;
}
EXPORT_SYMBOL_GPL(divider_determine_rate);

int divider_ro_determine_rate(struct clk_hw *hw, struct clk_rate_request *req,
			      const struct clk_div_table *table, u8 width,
			      unsigned long flags, unsigned int val)
{
	int div;

	div = _get_div(table, val, flags, width);

	/* Even a read-only clock can propagate a rate change */
	if (clk_hw_get_flags(hw) & CLK_SET_RATE_PARENT) {
		if (!req->best_parent_hw)
			return -EINVAL;

		req->best_parent_rate = clk_hw_round_rate(req->best_parent_hw,
							  req->rate * div);
	}

	req->rate = DIV_ROUND_UP_ULL((u64)req->best_parent_rate, div);

	return 0;
}
EXPORT_SYMBOL_GPL(divider_ro_determine_rate);

static int clk_divider_determine_rate(struct clk_hw *hw,
				      struct clk_rate_request *req)
{
	struct clk_divider *divider = to_clk_divider(hw);

	/* if read only, just return current value */
	if (divider->flags & CLK_DIVIDER_READ_ONLY) {
		u32 val;

		val = readl(divider->reg) >> divider->shift;
		val &= clk_div_mask(divider->width);

		return divider_ro_determine_rate(hw, req, divider->table,
						 divider->width,
						 divider->flags, val);
	}

	return divider_determine_rate(hw, req, divider->table, divider->width,
				      divider->flags);
}

int divider_get_val(unsigned long rate, unsigned long parent_rate,
		    const struct clk_div_table *table, u8 width,
		    unsigned long flags)
{
	unsigned int div, value;

	div = DIV_ROUND_UP_ULL((u64)parent_rate, rate);

	if (!_is_valid_div(table, div, flags))
		return -EINVAL;

	value = _get_val(table, div, flags);

	return min_t(unsigned int, value, clk_div_mask(width));
}

static int clk_divider_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_divider *divider = to_clk_divider(hw);
	int value;
	u32 val;

	if (divider->flags & CLK_DIVIDER_READ_ONLY)
		return 0;

	value = divider_get_val(rate, parent_rate, divider->table,
				divider->width, divider->flags);
	if (value < 0)
		return value;

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
	.determine_rate = clk_divider_determine_rate,
};

const struct clk_ops clk_divider_ro_ops = {
	.recalc_rate = clk_divider_recalc_rate,
	.determine_rate = clk_divider_determine_rate,
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

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014 Intel Corporation
 *
 * Adjustable fractional divider clock implementation.
 * Uses rational best approximation algorithm.
 *
 * Output is calculated as
 *
 *	rate = (m / n) * parent_rate				(1)
 *
 * This is useful when we have a prescaler block which asks for
 * m (numerator) and n (denominator) values to be provided to satisfy
 * the (1) as much as possible.
 *
 * Since m and n have the limitation by a range, e.g.
 *
 *	n >= 1, n < N_width, where N_width = 2^nwidth		(2)
 *
 * for some cases the output may be saturated. Hence, from (1) and (2),
 * assuming the worst case when m = 1, the inequality
 *
 *	floor(log2(parent_rate / rate)) <= nwidth		(3)
 *
 * may be derived. Thus, in cases when
 *
 *	(parent_rate / rate) >> N_width				(4)
 *
 * we might scale up the rate by 2^scale (see the description of
 * CLK_FRAC_DIVIDER_POWER_OF_TWO_PS for additional information), where
 *
 *	scale = floor(log2(parent_rate / rate)) - nwidth	(5)
 *
 * and assume that the IP, that needs m and n, has also its own
 * prescaler, which is capable to divide by 2^scale. In this way
 * we get the denominator to satisfy the desired range (2) and
 * at the same time a much better result of m and n than simple
 * saturated values.
 */

#include <common.h>
#include <linux/rational.h>

#include "clk-fractional-divider.h"

static inline u32 clk_fd_readl(struct clk_fractional_divider *fd)
{
	if (fd->flags & CLK_FRAC_DIVIDER_BIG_ENDIAN)
		return ioread32be(fd->reg);

	return readl(fd->reg);
}

static inline void clk_fd_writel(struct clk_fractional_divider *fd, u32 val)
{
	if (fd->flags & CLK_FRAC_DIVIDER_BIG_ENDIAN)
		iowrite32be(val, fd->reg);
	else
		writel(val, fd->reg);
}

static void clk_fd_get_div(struct clk_hw *hw, struct u32_fract *fract)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long m, n;
	u32 mmask, nmask;
	u32 val;

	val = clk_fd_readl(fd);

	mmask = GENMASK(fd->mwidth - 1, 0) << fd->mshift;
	nmask = GENMASK(fd->nwidth - 1, 0) << fd->nshift;

	m = (val & mmask) >> fd->mshift;
	n = (val & nmask) >> fd->nshift;

	if (fd->flags & CLK_FRAC_DIVIDER_ZERO_BASED) {
		m++;
		n++;
	}

	fract->numerator = m;
	fract->denominator = n;
}

static unsigned long clk_fd_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct u32_fract fract;
	u64 ret;

	clk_fd_get_div(hw, &fract);

	if (!fract.numerator || !fract.denominator)
		return parent_rate;

	ret = (u64)parent_rate * fract.numerator;
	do_div(ret, fract.denominator);

	return ret;
}

void clk_fractional_divider_general_approximation(struct clk_hw *hw,
						  unsigned long rate,
						  unsigned long *parent_rate,
						  unsigned long *m, unsigned long *n)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long max_m, max_n;

	/*
	 * Get rate closer to *parent_rate to guarantee there is no overflow
	 * for m and n. In the result it will be the nearest rate left shifted
	 * by (scale - fd->nwidth) bits.
	 *
	 * For the detailed explanation see the top comment in this file.
	 */
	if (fd->flags & CLK_FRAC_DIVIDER_POWER_OF_TWO_PS) {
		unsigned long scale = fls_long(*parent_rate / rate - 1);

		if (scale > fd->nwidth)
			rate <<= scale - fd->nwidth;
	}

	if (fd->flags & CLK_FRAC_DIVIDER_ZERO_BASED) {
		max_m = BIT(fd->mwidth);
		max_n = BIT(fd->nwidth);
	} else {
		max_m = GENMASK(fd->mwidth - 1, 0);
		max_n = GENMASK(fd->nwidth - 1, 0);
	}

	rational_best_approximation(rate, *parent_rate, max_m, max_n, m, n);
}
EXPORT_SYMBOL_GPL(clk_fractional_divider_general_approximation);

static long clk_fd_round_rate(struct clk_hw *hw, unsigned long rate,
			      unsigned long *parent_rate)
{
	struct clk *clk = clk_hw_to_clk(hw);
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long m, n;
	u64 ret;

	if (!rate || (!(clk->flags & CLK_SET_RATE_PARENT) && rate >= *parent_rate))
		return *parent_rate;

	if (fd->approximation)
		fd->approximation(hw, rate, parent_rate, &m, &n);
	else
		clk_fractional_divider_general_approximation(hw, rate, parent_rate, &m, &n);

	ret = (u64)*parent_rate * m;
	do_div(ret, n);

	return ret;
}

static int clk_fd_set_rate(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long m, n, max_m, max_n;
	u32 mmask, nmask;
	u32 val;

	if (fd->flags & CLK_FRAC_DIVIDER_ZERO_BASED) {
		max_m = BIT(fd->mwidth);
		max_n = BIT(fd->nwidth);
	} else {
		max_m = GENMASK(fd->mwidth - 1, 0);
		max_n = GENMASK(fd->nwidth - 1, 0);
	}
	rational_best_approximation(rate, parent_rate, max_m, max_n, &m, &n);

	if (fd->flags & CLK_FRAC_DIVIDER_ZERO_BASED) {
		m--;
		n--;
	}

	mmask = GENMASK(fd->mwidth - 1, 0) << fd->mshift;
	nmask = GENMASK(fd->nwidth - 1, 0) << fd->nshift;

	val = clk_fd_readl(fd);
	val &= ~(mmask | nmask);
	val |= (m << fd->mshift) | (n << fd->nshift);
	clk_fd_writel(fd, val);

	return 0;
}

const struct clk_ops clk_fractional_divider_ops = {
	.recalc_rate = clk_fd_recalc_rate,
	.round_rate = clk_fd_round_rate,
	.set_rate = clk_fd_set_rate,
};
EXPORT_SYMBOL_GPL(clk_fractional_divider_ops);

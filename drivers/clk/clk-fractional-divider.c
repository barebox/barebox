// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014 Intel Corporation
 *
 * Adjustable fractional divider clock implementation.
 * Output rate = (m / n) * parent_rate.
 * Uses rational best approximation algorithm.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/gcd.h>
#include <linux/math64.h>
#include <linux/rational.h>
#include <linux/barebox-wrapper.h>

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

static unsigned long clk_fd_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long m, n;
	u32 val;
	u64 ret;

	val = clk_fd_readl(fd);

	m = (val & fd->mmask) >> fd->mshift;
	n = (val & fd->nmask) >> fd->nshift;

	if (fd->flags & CLK_FRAC_DIVIDER_ZERO_BASED) {
		m++;
		n++;
	}

	if (!n || !m)
		return parent_rate;

	ret = (u64)parent_rate * m;
	do_div(ret, n);

	return ret;
}

static void clk_fd_general_approximation(struct clk_hw *hw, unsigned long rate,
					 unsigned long *parent_rate,
					 unsigned long *m, unsigned long *n)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long scale;

	/*
	 * Get rate closer to *parent_rate to guarantee there is no overflow
	 * for m and n. In the result it will be the nearest rate left shifted
	 * by (scale - fd->nwidth) bits.
	 */
	scale = fls_long(*parent_rate / rate - 1);
	if (scale > fd->nwidth)
		rate <<= scale - fd->nwidth;

	rational_best_approximation(rate, *parent_rate,
			GENMASK(fd->mwidth - 1, 0), GENMASK(fd->nwidth - 1, 0),
			m, n);
}

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
		clk_fd_general_approximation(hw, rate, parent_rate, &m, &n);

	ret = (u64)*parent_rate * m;
	do_div(ret, n);

	return ret;
}

static int clk_fd_set_rate(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long m, n;
	u32 val;

	rational_best_approximation(rate, parent_rate,
			GENMASK(fd->mwidth - 1, 0), GENMASK(fd->nwidth - 1, 0),
			&m, &n);

	if (fd->flags & CLK_FRAC_DIVIDER_ZERO_BASED) {
		m--;
		n--;
	}

	val = clk_fd_readl(fd);
	val &= ~(fd->mmask | fd->nmask);
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

struct clk *clk_fractional_divider_alloc(
		const char *name, const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 mshift, u8 mwidth, u8 nshift, u8 nwidth,
		u8 clk_divider_flags)
{
	struct clk_fractional_divider *fd;

	fd = xzalloc(sizeof(*fd));

	fd->reg = reg;
	fd->mshift = mshift;
	fd->mwidth = mwidth;
	fd->mmask = GENMASK(mwidth - 1, 0) << mshift;
	fd->nshift = nshift;
	fd->nwidth = nwidth;
	fd->nmask = GENMASK(nwidth - 1, 0) << nshift;
	fd->flags = clk_divider_flags;
	fd->hw.clk.name = name;
	fd->hw.clk.ops = &clk_fractional_divider_ops;
	fd->hw.clk.flags = flags;
	fd->hw.clk.parent_names = parent_name ? &parent_name : NULL;
	fd->hw.clk.num_parents = parent_name ? 1 : 0;

	return &fd->hw.clk;
}

void clk_fractional_divider_free(struct clk *clk_fd)
{
	struct clk_fractional_divider *fd = to_clk_fd(clk_to_clk_hw(clk_fd));

	free(fd);
}

struct clk *clk_fractional_divider(
		const char *name, const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 mshift, u8 mwidth, u8 nshift, u8 nwidth,
		u8 clk_divider_flags)
{
	struct clk *fd;
	int ret;

	fd = clk_fractional_divider_alloc(name, parent_name, flags,
		reg, mshift, mwidth, nshift, nwidth,
		clk_divider_flags);

	if (IS_ERR(fd))
		return fd;

	ret = bclk_register(fd);
	if (ret) {
		clk_fractional_divider_free(fd);
		return ERR_PTR(ret);
	}

	return fd;
}
EXPORT_SYMBOL_GPL(clk_fractional_divider);

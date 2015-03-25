/*
 * Copyright (C) 2014 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable fractional divider clock implementation.
 * Output rate = (m / n) * parent_rate.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gcd.h>
#include <linux/math64.h>
#include <linux/barebox-wrapper.h>

#define to_clk_fd(_hw) container_of(_hw, struct clk_fractional_divider, clk)

struct clk_fractional_divider {
	struct clk	clk;
	void __iomem	*reg;
	u8		mshift;
	u32		mmask;
	u8		nshift;
	u32		nmask;
	u8		flags;
};

static unsigned long clk_fd_recalc_rate(struct clk *hw,
					unsigned long parent_rate)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	u32 val, m, n;
	u64 ret;

	val = readl(fd->reg);

	m = (val & fd->mmask) >> fd->mshift;
	n = (val & fd->nmask) >> fd->nshift;

	ret = (u64)parent_rate * m;
	do_div(ret, n);

	return ret;
}

static long clk_fd_round_rate(struct clk *hw, unsigned long rate,
			      unsigned long *prate)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned maxn = (fd->nmask >> fd->nshift) + 1;
	unsigned div;

	if (!rate || rate >= *prate)
		return *prate;

	div = gcd(*prate, rate);

	while ((*prate / div) > maxn) {
		div <<= 1;
		rate <<= 1;
	}

	return rate;
}

static int clk_fd_set_rate(struct clk *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long div;
	unsigned n, m;
	u32 val;

	div = gcd(parent_rate, rate);
	m = rate / div;
	n = parent_rate / div;

	val = readl(fd->reg);
	val &= ~(fd->mmask | fd->nmask);
	val |= (m << fd->mshift) | (n << fd->nshift);
	writel(val, fd->reg);

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
	fd->mmask = (BIT(mwidth) - 1) << mshift;
	fd->nshift = nshift;
	fd->nmask = (BIT(nwidth) - 1) << nshift;
	fd->flags = clk_divider_flags;
	fd->clk.name = name;
	fd->clk.ops = &clk_fractional_divider_ops;
	fd->clk.flags = flags;
	fd->clk.parent_names = parent_name ? &parent_name : NULL;
	fd->clk.num_parents = parent_name ? 1 : 0;

	return &fd->clk;
}

void clk_fractional_divider_free(struct clk *clk_fd)
{
	struct clk_fractional_divider *fd = to_clk_fd(clk_fd);

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

	ret = clk_register(fd);
	if (ret) {
		clk_fractional_divider_free(fd);
		return ERR_PTR(ret);
	}

	return fd;
}
EXPORT_SYMBOL_GPL(clk_fractional_divider);

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <malloc.h>
#include <clock.h>
#include <asm-generic/div64.h>

#include "clk.h"

#define PLL_NUM_OFFSET		0x10
#define PLL_DENOM_OFFSET	0x20

#define SYS_VF610_PLL_OFFSET	0x10

#define BM_PLL_POWER		(0x1 << 12)
#define BM_PLL_ENABLE		(0x1 << 13)
#define BM_PLL_BYPASS		(0x1 << 16)
#define BM_PLL_LOCK		(0x1 << 31)
#define IMX7_ENET_PLL_POWER	(0x1 << 5)

struct clk_pllv3 {
	struct clk	clk;
	void __iomem	*base;
	bool		powerup_set;
	u32		div_mask;
	u32		div_shift;
	const char	*parent;
	u32		ref_clock;
	u32		power_bit;
};

#define to_clk_pllv3(_clk) container_of(_clk, struct clk_pllv3, clk)

static int clk_pllv3_enable(struct clk *clk)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	u32 val;
	int timeout = 10000;

	val = readl(pll->base);
	if (pll->powerup_set)
		val |= pll->power_bit;
	else
		val &= ~pll->power_bit;
	writel(val, pll->base);

	/* Wait for PLL to lock */
	while (timeout--) {
		if (readl(pll->base) & BM_PLL_LOCK)
			break;
	}

	if (!timeout)
		return -ETIMEDOUT;

	val = readl(pll->base);
	val |= BM_PLL_ENABLE;
	writel(val, pll->base);

	return 0;
}

static void clk_pllv3_disable(struct clk *clk)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	u32 val;

	val = readl(pll->base);
	val &= ~BM_PLL_ENABLE;
	writel(val, pll->base);

	if (pll->powerup_set)
		val &= ~pll->power_bit;
	else
		val |= pll->power_bit;
	writel(val, pll->base);
}

static unsigned long clk_pllv3_recalc_rate(struct clk *clk,
					   unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	u32 div = (readl(pll->base) >> pll->div_shift) & pll->div_mask;

	return (div == 1) ? parent_rate * 22 : parent_rate * 20;
}

static long clk_pllv3_round_rate(struct clk *clk, unsigned long rate,
				 unsigned long *prate)
{
	unsigned long parent_rate = *prate;

	return (rate >= parent_rate * 22) ? parent_rate * 22 :
					    parent_rate * 20;
}

static int clk_pllv3_set_rate(struct clk *clk, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	u32 val, div;

	if (rate == parent_rate * 22)
		div = 1;
	else if (rate == parent_rate * 20)
		div = 0;
	else
		return -EINVAL;

	val = readl(pll->base);
	val &= ~(pll->div_mask << pll->div_shift);
	val |= div << pll->div_shift;
	writel(val, pll->base);

	return 0;
}

static const struct clk_ops clk_pllv3_ops = {
	.enable		= clk_pllv3_enable,
	.disable	= clk_pllv3_disable,
	.recalc_rate	= clk_pllv3_recalc_rate,
	.round_rate	= clk_pllv3_round_rate,
	.set_rate	= clk_pllv3_set_rate,
};

static unsigned long clk_pllv3_sys_recalc_rate(struct clk *clk,
					       unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	u32 div = readl(pll->base) & pll->div_mask;

	return parent_rate * div / 2;
}

static long clk_pllv3_sys_round_rate(struct clk *clk, unsigned long rate,
				     unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	unsigned long min_rate = parent_rate * 54 / 2;
	unsigned long max_rate = parent_rate * 108 / 2;
	u32 div;

	if (rate > max_rate)
		rate = max_rate;
	else if (rate < min_rate)
		rate = min_rate;
	div = rate * 2 / parent_rate;

	return parent_rate * div / 2;
}

static int clk_pllv3_sys_set_rate(struct clk *clk, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	unsigned long min_rate = parent_rate * 54 / 2;
	unsigned long max_rate = parent_rate * 108 / 2;
	u32 val, div;

	if (rate < min_rate || rate > max_rate)
		return -EINVAL;

	div = rate * 2 / parent_rate;
	val = readl(pll->base);
	val &= ~pll->div_mask;
	val |= div;
	writel(val, pll->base);

	return 0;
}

static const struct clk_ops clk_pllv3_sys_ops = {
	.enable		= clk_pllv3_enable,
	.disable	= clk_pllv3_disable,
	.recalc_rate	= clk_pllv3_sys_recalc_rate,
	.round_rate	= clk_pllv3_sys_round_rate,
	.set_rate	= clk_pllv3_sys_set_rate,
};

static unsigned long clk_pllv3_av_recalc_rate(struct clk *clk,
					      unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	u32 mfn = readl(pll->base + PLL_NUM_OFFSET);
	u32 mfd = readl(pll->base + PLL_DENOM_OFFSET);
	u32 div = readl(pll->base) & pll->div_mask;

	return (parent_rate * div) + ((parent_rate / mfd) * mfn);
}

static long clk_pllv3_av_round_rate(struct clk *clk, unsigned long rate,
				    unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	unsigned long min_rate = parent_rate * 27;
	unsigned long max_rate = parent_rate * 54;
	u32 div;
	u32 mfn, mfd = 1000000;
	u64 temp64;

	if (rate > max_rate)
		rate = max_rate;
	else if (rate < min_rate)
		rate = min_rate;

	div = rate / parent_rate;
	temp64 = (u64) (rate - div * parent_rate);
	temp64 *= mfd;
	do_div(temp64, parent_rate);
	mfn = temp64;

	return parent_rate * div + parent_rate / mfd * mfn;
}

static int clk_pllv3_av_set_rate(struct clk *clk, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	unsigned long min_rate = parent_rate * 27;
	unsigned long max_rate = parent_rate * 54;
	u32 val, div;
	u32 mfn, mfd = 1000000;
	u64 temp64;

	if (rate < min_rate || rate > max_rate)
		return -EINVAL;

	div = rate / parent_rate;
	temp64 = (u64) (rate - div * parent_rate);
	temp64 *= mfd;
	do_div(temp64, parent_rate);
	mfn = temp64;

	val = readl(pll->base);
	val &= ~pll->div_mask;
	val |= div;
	writel(val, pll->base);
	writel(mfn, pll->base + PLL_NUM_OFFSET);
	writel(mfd, pll->base + PLL_DENOM_OFFSET);

	return 0;
}

static const struct clk_ops clk_pllv3_av_ops = {
	.enable		= clk_pllv3_enable,
	.disable	= clk_pllv3_disable,
	.recalc_rate	= clk_pllv3_av_recalc_rate,
	.round_rate	= clk_pllv3_av_round_rate,
	.set_rate	= clk_pllv3_av_set_rate,
};

static unsigned long clk_pllv3_enet_recalc_rate(struct clk *clk,
						unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);

	return pll->ref_clock;
}

static const struct clk_ops clk_pllv3_enet_ops = {
	.enable		= clk_pllv3_enable,
	.disable	= clk_pllv3_disable,
	.recalc_rate	= clk_pllv3_enet_recalc_rate,
};

static const struct clk_ops clk_pllv3_mlb_ops = {
	.enable		= clk_pllv3_enable,
	.disable	= clk_pllv3_disable,
};

static unsigned long clk_pllv3_sys_vf610_recalc_rate(struct clk *clk,
						     unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);

	u32 mfn = readl(pll->base + SYS_VF610_PLL_OFFSET + PLL_NUM_OFFSET);
	u32 mfd = readl(pll->base + SYS_VF610_PLL_OFFSET + PLL_DENOM_OFFSET);
	u32 div = (readl(pll->base) & pll->div_mask) ? 22 : 20;

	return (parent_rate * div) + ((parent_rate / mfd) * mfn);
}

static long clk_pllv3_sys_vf610_round_rate(struct clk *clk, unsigned long rate,
					   unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	unsigned long min_rate = parent_rate * 20;
	unsigned long max_rate = 528000000;
	u32 mfn, mfd = 1000000;
	u64 temp64;

	if (rate >= max_rate)
		return max_rate;
	else if (rate < min_rate)
		rate = min_rate;

	temp64 = (u64) (rate - 20 * parent_rate);
	temp64 *= mfd;
	do_div(temp64, parent_rate);
	mfn = temp64;

	return parent_rate * 20 + parent_rate / mfd * mfn;
}

static int clk_pllv3_sys_vf610_set_rate(struct clk *clk, unsigned long rate,
					unsigned long parent_rate)
{
	struct clk_pllv3 *pll = to_clk_pllv3(clk);
	unsigned long min_rate = parent_rate * 20;
	unsigned long max_rate = 528000000;
	u32 val;
	u32 mfn, mfd = 1000000;
	u64 temp64;

	if (rate < min_rate || rate > max_rate)
		return -EINVAL;

	val = readl(pll->base);

	if (rate == max_rate) {
		writel(0, pll->base + SYS_VF610_PLL_OFFSET + PLL_NUM_OFFSET);
		val |= pll->div_mask;
		writel(val, pll->base);

		return 0;
	} else {
		val &= ~pll->div_mask;
	}

	temp64 = (u64) (rate - 20 * parent_rate);
	temp64 *= mfd;
	do_div(temp64, parent_rate);
	mfn = temp64;

	writel(val, pll->base);
	writel(mfn, pll->base + SYS_VF610_PLL_OFFSET + PLL_NUM_OFFSET);
	writel(mfd, pll->base + SYS_VF610_PLL_OFFSET + PLL_DENOM_OFFSET);

	return 0;
}

static const struct clk_ops clk_pllv3_sys_vf610_ops = {
	.enable		= clk_pllv3_enable,
	.disable	= clk_pllv3_disable,
	.recalc_rate	= clk_pllv3_sys_vf610_recalc_rate,
	.round_rate	= clk_pllv3_sys_vf610_round_rate,
	.set_rate	= clk_pllv3_sys_vf610_set_rate,
};

struct clk *imx_clk_pllv3(enum imx_pllv3_type type, const char *name,
			  const char *parent, void __iomem *base,
			  u32 div_mask)
{
	struct clk_pllv3 *pll;
	const struct clk_ops *ops;
	int ret;

	pll = xzalloc(sizeof(*pll));

	pll->power_bit = BM_PLL_POWER;

	switch (type) {
	case IMX_PLLV3_SYS_VF610:
		ops = &clk_pllv3_sys_vf610_ops;
		break;
	case IMX_PLLV3_SYS:
		ops = &clk_pllv3_sys_ops;
		break;
	case IMX_PLLV3_USB_VF610:
		pll->div_shift = 1;
	case IMX_PLLV3_USB:
		ops = &clk_pllv3_ops;
		pll->powerup_set = true;
		break;
	case IMX_PLLV3_AV:
		ops = &clk_pllv3_av_ops;
		break;
	case IMX_PLLV3_ENET_IMX7:
		pll->power_bit = IMX7_ENET_PLL_POWER;
		pll->ref_clock = 1000000000;
		ops = &clk_pllv3_enet_ops;
		break;
	case IMX_PLLV3_ENET:
		pll->ref_clock = 500000000;
		ops = &clk_pllv3_enet_ops;
		break;
	case IMX_PLLV3_MLB:
		ops = &clk_pllv3_mlb_ops;
		break;
	default:
		ops = &clk_pllv3_ops;
	}
	pll->base = base;
	pll->div_mask = div_mask;
	pll->parent = parent;
	pll->clk.ops = ops;
	pll->clk.name = name;
	pll->clk.parent_names = &pll->parent;
	pll->clk.num_parents = 1;

	ret = clk_register(&pll->clk);
	if (ret) {
		free(pll);
		return ERR_PTR(ret);
	}

	return &pll->clk;
}

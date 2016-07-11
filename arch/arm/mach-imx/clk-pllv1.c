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
#include <asm-generic/div64.h>

#include "clk.h"

struct clk_pllv1 {
	struct clk clk;
	void __iomem *reg;
	const char *parent;
};

static unsigned long clk_pllv1_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	struct clk_pllv1 *pll = container_of(clk, struct clk_pllv1, clk);
	unsigned long long ll;
	int mfn_abs;
	unsigned int mfi, mfn, mfd, pd;
	u32 reg_val = readl(pll->reg);
	unsigned long freq = parent_rate;

	mfi = (reg_val >> 10) & 0xf;
	mfn = reg_val & 0x3ff;
	mfd = (reg_val >> 16) & 0x3ff;
	pd =  (reg_val >> 26) & 0xf;

	mfi = mfi <= 5 ? 5 : mfi;

	mfn_abs = mfn;

#if !defined CONFIG_ARCH_MX1 && !defined CONFIG_ARCH_MX21
	if (mfn >= 0x200) {
		mfn |= 0xFFFFFE00;
		mfn_abs = -mfn;
	}
#endif

	freq *= 2;
	freq /= pd + 1;

	ll = (unsigned long long)freq * mfn_abs;

	do_div(ll, mfd + 1);
	if (mfn < 0)
		ll = (freq * mfi) - ll;
	else
		ll = (freq * mfi) + ll;

	return ll;
}

struct clk_ops clk_pllv1_ops = {
	.recalc_rate = clk_pllv1_recalc_rate,
};

struct clk *imx_clk_pllv1(const char *name, const char *parent,
		void __iomem *base)
{
	struct clk_pllv1 *pll = xzalloc(sizeof(*pll));
	int ret;

	pll->parent = parent;
	pll->reg = base;
	pll->clk.ops = &clk_pllv1_ops;
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

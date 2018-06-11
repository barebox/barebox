/*
 * Copyright 2017 NXP.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/div64.h>

#include "clk.h"

#define PLL_CFG0 	0x0
#define PLL_CFG1	0x4

#define PLL_LOCK_STATUS	(0x1 << 31)
#define PLL_CLKE	21
#define PLL_PD		19
#define PLL_BYPASS	14
#define PLL_NEWDIV_VAL		(1 << 12)
#define PLL_NEWDIV_ACK		(1 << 11)
#define PLL_FRAC_DIV_MASK	0xffffff
#define PLL_INT_DIV_MASK	0x7f
#define PLL_FRAC_DENOM		0x1000000

struct clk_frac_pll {
	struct clk	clk;
	void __iomem	*base;
	const char *parent;
};

#define to_clk_frac_pll(_clk) container_of(_clk, struct clk_frac_pll, clk)

static int clk_wait_lock(struct clk_frac_pll *pll)
{
	unsigned long timeout = 100000;

	/* Wait for PLL to lock */
	while (timeout--) {
		if (readl(pll->base) & PLL_LOCK_STATUS)
			break;
	}

	return readl(pll->base) & PLL_LOCK_STATUS ? 0 : -ETIMEDOUT;
}

static int clk_wait_ack(struct clk_frac_pll *pll)
{
	unsigned long timeout = 100000;

	/* return directly if the pll is in powerdown or in bypass */
	if (readl(pll->base) & ((1 << PLL_PD) | (1 << PLL_BYPASS)))
		return 0;

	/* Wait for the pll's divfi and divff to be reloaded */
	while (timeout--) {
		if (readl(pll->base) & PLL_NEWDIV_ACK)
			break;
	}

	return readl(pll->base) & PLL_NEWDIV_ACK ? 0 : ETIMEDOUT;
}

static int clk_pll_enable(struct clk *clk)
{
	struct clk_frac_pll *pll = to_clk_frac_pll(clk);
	u32 val;

	val = readl(pll->base + PLL_CFG0);
	val &= ~(1 << PLL_PD);
	writel(val, pll->base + PLL_CFG0);

	return clk_wait_lock(pll);
}

static void clk_pll_disable(struct clk *clk)
{
	struct clk_frac_pll *pll = to_clk_frac_pll(clk);
	u32 val;

	val = readl(pll->base + PLL_CFG0);
	val |= (1 << PLL_PD);
	writel(val, pll->base + PLL_CFG0);
}

static int clk_pll_is_enabled(struct clk *clk)
{
	struct clk_frac_pll *pll = to_clk_frac_pll(clk);
	u32 val;

	val = readl(pll->base + PLL_CFG0);
	return (val & (1 << PLL_PD)) ? 0 : 1;
}

static unsigned long clk_pll_recalc_rate(struct clk *clk,
					 unsigned long parent_rate)
{
	struct clk_frac_pll *pll = to_clk_frac_pll(clk);
	u32 val, divff, divfi, divq;
	u64 temp64;

	val = readl(pll->base + PLL_CFG0);
	divq = ((val & 0x1f) + 1) * 2;
	val = readl(pll->base + PLL_CFG1);
	divff = (val >> 7) & PLL_FRAC_DIV_MASK;
	divfi = (val & PLL_INT_DIV_MASK);

	temp64 = (u64)parent_rate * 8;
	temp64 *= divff;
	do_div(temp64, PLL_FRAC_DENOM);
	temp64 /= divq;

	return parent_rate * 8 * (divfi + 1) / divq + (unsigned long)temp64;
}

static long clk_pll_round_rate(struct clk *clk, unsigned long rate,
			       unsigned long *prate)
{
	u32 divff, divfi;
	u64 temp64;
	unsigned long parent_rate = *prate;

	parent_rate *= 8;
	rate *= 2;
	divfi = rate / parent_rate;
	temp64 = (u64)(rate - divfi * parent_rate);
	temp64 *= PLL_FRAC_DENOM;
	do_div(temp64, parent_rate);
	divff = temp64;

	temp64 = (u64)parent_rate;
	temp64 *= divff;
	do_div(temp64, PLL_FRAC_DENOM);

	return (parent_rate * divfi + (unsigned long)temp64) / 2;
}

/*
 * To simplify the clock calculation, we can keep the 'PLL_OUTPUT_VAL' at zero
 * (means the PLL output will be divided by 2). So the PLL output can use
 * the below formula:
 * pllout = parent_rate * 8 / 2 * DIVF_VAL;
 * where DIVF_VAL = 1 + DIVFI + DIVFF / 2^24.
 */
static int clk_pll_set_rate(struct clk *clk, unsigned long rate,
			    unsigned long parent_rate)
{
	struct clk_frac_pll *pll = to_clk_frac_pll(clk);
	u32 val, divfi, divff;
	u64 temp64;
	int ret;

	parent_rate *= 8;
	rate *= 2;
	divfi = rate / parent_rate;
	temp64 = (u64) (rate - divfi * parent_rate);
	temp64 *= PLL_FRAC_DENOM;
	do_div(temp64, parent_rate);
	divff = temp64;

	val = readl(pll->base + PLL_CFG1);
	val &= ~((PLL_FRAC_DIV_MASK << 7) | (PLL_INT_DIV_MASK));
	val |= ((divff << 7) | (divfi - 1));
	writel(val, pll->base + PLL_CFG1);

	val = readl(pll->base + PLL_CFG0);
	val &= ~0x1f;
	writel(val, pll->base + PLL_CFG0);

	/* Set the NEV_DIV_VAL to reload the DIVFI and DIVFF */
	val = readl(pll->base + PLL_CFG0);
	val |= PLL_NEWDIV_VAL;
	writel(val, pll->base + PLL_CFG0);

	ret = clk_wait_ack(pll);

	/* clear the NEV_DIV_VAL */
	val = readl(pll->base + PLL_CFG0);
	val &= ~PLL_NEWDIV_VAL;
	writel(val, pll->base + PLL_CFG0);

	return ret;
}

static const struct clk_ops clk_frac_pll_ops = {
	.enable		= clk_pll_enable,
	.disable	= clk_pll_disable,
	.is_enabled	= clk_pll_is_enabled,
	.recalc_rate	= clk_pll_recalc_rate,
	.round_rate	= clk_pll_round_rate,
	.set_rate	= clk_pll_set_rate,
};

struct clk *imx_clk_frac_pll(const char *name, const char *parent,
			     void __iomem *base)
{
	struct clk_frac_pll *pll;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	pll->base = base;
	pll->parent = parent;
	pll->clk.ops = &clk_frac_pll_ops;
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

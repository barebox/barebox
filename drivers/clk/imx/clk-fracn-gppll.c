// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2021 NXP
 */

#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/types.h>
#include <of_address.h>
#include <linux/iopoll.h>
#include <linux/bitfield.h>
#include <soc/imx/clk-fracn-gppll.h>

#include "clk.h"

#define PLL_FRACN_GP(_rate, _mfi, _mfn, _mfd, _rdiv, _odiv)	\
	{							\
		.rate	=	(_rate),			\
		.mfi	=	(_mfi),				\
		.mfn	=	(_mfn),				\
		.mfd	=	(_mfd),				\
		.rdiv	=	(_rdiv),			\
		.odiv	=	(_odiv),			\
	}

#define PLL_FRACN_GP_INTEGER(_rate, _mfi, _rdiv, _odiv)		\
	{							\
		.rate	=	(_rate),			\
		.mfi	=	(_mfi),				\
		.mfn	=	0,				\
		.mfd	=	0,				\
		.rdiv	=	(_rdiv),			\
		.odiv	=	(_odiv),			\
	}

struct clk_fracn_gppll {
	struct clk_hw			hw;
	void __iomem			*base;
	const struct imx_fracn_gppll_rate_table *rate_table;
	int rate_count;
	u32 flags;
};

/*
 * Fvco = (Fref / rdiv) * (MFI + MFN / MFD)
 * Fout = Fvco / odiv
 * The (Fref / rdiv) should be in range 20MHz to 40MHz
 * The Fvco should be in range 2.5Ghz to 5Ghz
 */
static const struct imx_fracn_gppll_rate_table fracn_tbl[] = {
	PLL_FRACN_GP(650000000U, 162, 50, 100, 0, 6),
	PLL_FRACN_GP(594000000U, 198, 0, 1, 0, 8),
	PLL_FRACN_GP(560000000U, 140, 0, 1, 0, 6),
	PLL_FRACN_GP(519750000U, 173, 25, 100, 1, 8),
	PLL_FRACN_GP(498000000U, 166, 0, 1, 0, 8),
	PLL_FRACN_GP(484000000U, 121, 0, 1, 0, 6),
	PLL_FRACN_GP(445333333U, 167, 0, 1, 0, 9),
	PLL_FRACN_GP(400000000U, 200, 0, 1, 0, 12),
	PLL_FRACN_GP(393216000U, 163, 84, 100, 0, 10),
	PLL_FRACN_GP(300000000U, 150, 0, 1, 0, 12)
};

struct imx_fracn_gppll_clk imx_fracn_gppll = {
	.rate_table = fracn_tbl,
	.rate_count = ARRAY_SIZE(fracn_tbl),
};
EXPORT_SYMBOL_GPL(imx_fracn_gppll);

/*
 * Fvco = (Fref / rdiv) * MFI
 * Fout = Fvco / odiv
 * The (Fref / rdiv) should be in range 20MHz to 40MHz
 * The Fvco should be in range 2.5Ghz to 5Ghz
 */
static const struct imx_fracn_gppll_rate_table int_tbl[] = {
	PLL_FRACN_GP_INTEGER(1700000000U, 141, 1, 2),
	PLL_FRACN_GP_INTEGER(1400000000U, 175, 1, 3),
	PLL_FRACN_GP_INTEGER(900000000U, 150, 1, 4),
};

struct imx_fracn_gppll_clk imx_fracn_gppll_integer = {
	.rate_table = int_tbl,
	.rate_count = ARRAY_SIZE(int_tbl),
};
EXPORT_SYMBOL_GPL(imx_fracn_gppll_integer);

static inline struct clk_fracn_gppll *to_clk_fracn_gppll(struct clk_hw *hw)
{
	return container_of(hw, struct clk_fracn_gppll, hw);
}

static long clk_fracn_gppll_round_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long *prate)
{
	struct clk_fracn_gppll *pll = to_clk_fracn_gppll(hw);
	const struct imx_fracn_gppll_rate_table *rate_table = pll->rate_table;
	int i;

	/* Assuming rate_table is in descending order */
	for (i = 0; i < pll->rate_count; i++)
		if (rate >= rate_table[i].rate)
			return rate_table[i].rate;

	/* return minimum supported value */
	return rate_table[pll->rate_count - 1].rate;
}

static unsigned long clk_fracn_gppll_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct clk_fracn_gppll *pll = to_clk_fracn_gppll(hw);
	const struct imx_fracn_gppll_rate_table *rate_table = pll->rate_table;
	u32 pll_numerator, pll_denominator, pll_div;
	u32 mfi, mfn, mfd, rdiv, odiv;
	u64 fvco = parent_rate;
	long rate = 0;
	int i;

	pll_numerator = readl_relaxed(pll->base + GPPLL_NUMERATOR);
	mfn = FIELD_GET(GPPLL_MFN_MASK, pll_numerator);

	pll_denominator = readl_relaxed(pll->base + GPPLL_DENOMINATOR);
	mfd = FIELD_GET(GPPLL_MFD_MASK, pll_denominator);

	pll_div = readl_relaxed(pll->base + GPPLL_DIV);
	mfi = FIELD_GET(GPPLL_MFI_MASK, pll_div);

	rdiv = FIELD_GET(GPPLL_RDIV_MASK, pll_div);
	odiv = FIELD_GET(GPPLL_ODIV_MASK, pll_div);

	/*
	 * Sometimes, the recalculated rate has deviation due to
	 * the frac part. So find the accurate pll rate from the table
	 * first, if no match rate in the table, use the rate calculated
	 * from the equation below.
	 */
	for (i = 0; i < pll->rate_count; i++) {
		if (rate_table[i].mfn == mfn && rate_table[i].mfi == mfi &&
		    rate_table[i].mfd == mfd && rate_table[i].rdiv == rdiv &&
		    rate_table[i].odiv == odiv)
			rate = rate_table[i].rate;
	}

	if (rate)
		return (unsigned long)rate;

	if (!rdiv)
		rdiv = rdiv + 1;

	switch (odiv) {
	case 0:
		odiv = 2;
		break;
	case 1:
		odiv = 3;
		break;
	default:
		break;
	}

	if (pll->flags & CLK_FRACN_GPPLL_INTEGER) {
		/* Fvco = (Fref / rdiv) * MFI */
		fvco = fvco * mfi;
		do_div(fvco, rdiv * odiv);
	} else {
		/* Fvco = (Fref / rdiv) * (MFI + MFN / MFD) */
		fvco = fvco * mfi * mfd + fvco * mfn;
		do_div(fvco, mfd * rdiv * odiv);
	}

	return (unsigned long)fvco;
}

static int clk_fracn_gppll_wait_lock(struct clk_fracn_gppll *pll)
{
	return fracn_gppll_wait_lock(pll->base);
}

static int clk_fracn_gppll_set_rate(struct clk_hw *hw, unsigned long drate,
				    unsigned long prate)
{
	struct clk_fracn_gppll *pll = to_clk_fracn_gppll(hw);

	return fracn_gppll_set_rate(pll->base, pll->flags, pll->rate_table,
				    pll->rate_count, drate);
}

static int clk_fracn_gppll_prepare(struct clk_hw *hw)
{
	struct clk_fracn_gppll *pll = to_clk_fracn_gppll(hw);
	u32 val;
	int ret;

	val = readl_relaxed(pll->base + GPPLL_CTRL);
	if (val & POWERUP_MASK)
		return 0;

	val |= CLKMUX_BYPASS;
	writel_relaxed(val, pll->base + GPPLL_CTRL);

	val |= POWERUP_MASK;
	writel_relaxed(val, pll->base + GPPLL_CTRL);

	val |= CLKMUX_EN;
	writel_relaxed(val, pll->base + GPPLL_CTRL);

	ret = clk_fracn_gppll_wait_lock(pll);
	if (ret)
		return ret;

	val &= ~CLKMUX_BYPASS;
	writel_relaxed(val, pll->base + GPPLL_CTRL);

	return 0;
}

static int clk_fracn_gppll_is_prepared(struct clk_hw *hw)
{
	struct clk_fracn_gppll *pll = to_clk_fracn_gppll(hw);
	u32 val;

	val = readl_relaxed(pll->base + GPPLL_CTRL);

	return (val & POWERUP_MASK) ? 1 : 0;
}

static void clk_fracn_gppll_unprepare(struct clk_hw *hw)
{
	struct clk_fracn_gppll *pll = to_clk_fracn_gppll(hw);
	u32 val;

	val = readl_relaxed(pll->base + GPPLL_CTRL);
	val &= ~POWERUP_MASK;
	writel_relaxed(val, pll->base + GPPLL_CTRL);
}

static const struct clk_ops clk_fracn_gppll_ops = {
	.enable		= clk_fracn_gppll_prepare,
	.disable	= clk_fracn_gppll_unprepare,
	.is_enabled	= clk_fracn_gppll_is_prepared,
	.recalc_rate	= clk_fracn_gppll_recalc_rate,
	.round_rate	= clk_fracn_gppll_round_rate,
	.set_rate	= clk_fracn_gppll_set_rate,
};

static struct clk *_imx_clk_fracn_gppll(const char *name, const char *parent_name,
					void __iomem *base,
					const struct imx_fracn_gppll_clk *pll_clk,
					u32 pll_flags)
{
	struct clk_fracn_gppll *pll;
	struct clk_hw *hw;
	struct clk_init_data init;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.flags = pll_clk->flags;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.ops = &clk_fracn_gppll_ops;

	pll->base = base;
	pll->hw.init = &init;
	pll->rate_table = pll_clk->rate_table;
	pll->rate_count = pll_clk->rate_count;
	pll->flags = pll_flags;

	hw = &pll->hw;

	ret = clk_hw_register(NULL, hw);
	if (ret) {
		pr_err("%s: failed to register pll %s %d\n", __func__, name, ret);
		kfree(pll);
		return ERR_PTR(ret);
	}

	return &hw->clk;
}

struct clk *imx_clk_fracn_gppll(const char *name, const char *parent_name, void __iomem *base,
				const struct imx_fracn_gppll_clk *pll_clk)
{
	return _imx_clk_fracn_gppll(name, parent_name, base, pll_clk, CLK_FRACN_GPPLL_FRACN);
}
EXPORT_SYMBOL_GPL(imx_clk_fracn_gppll);

struct clk *imx_clk_fracn_gppll_integer(const char *name, const char *parent_name,
					void __iomem *base,
					const struct imx_fracn_gppll_clk *pll_clk)
{
	return _imx_clk_fracn_gppll(name, parent_name, base, pll_clk, CLK_FRACN_GPPLL_INTEGER);
}
EXPORT_SYMBOL_GPL(imx_clk_fracn_gppll_integer);

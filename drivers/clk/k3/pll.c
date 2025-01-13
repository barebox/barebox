// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments K3 SoC PLL clock driver
 *
 * Copyright (C) 2020-2021 Texas Instruments Incorporated - https://www.ti.com/
 *	Tero Kristo <t-kristo@ti.com>
 */

#include <io.h>
#include <errno.h>
#include <linux/clk-provider.h>
#include <linux/rational.h>
#include <linux/slab.h>
#include <linux/bitmap.h>
#include <linux/printk.h>
#include <soc/k3/clk.h>

#include "ti-k3-clk.h"

/* 16FFT register offsets */
#define PLL_16FFT_CFG			0x08
#define PLL_KICK0			0x10
#define PLL_KICK1			0x14
#define PLL_16FFT_CTRL			0x20
#define PLL_16FFT_STAT			0x24
#define PLL_16FFT_FREQ_CTRL0		0x30
#define PLL_16FFT_FREQ_CTRL1		0x34
#define PLL_16FFT_DIV_CTRL		0x38
#define PLL_16FFT_CAL_CTRL		0x60
#define PLL_16FFT_CAL_STAT		0x64

/* CAL STAT register bits */
#define PLL_16FFT_CAL_STAT_CAL_LOCK	BIT(31)

/* CFG register bits */
#define PLL_16FFT_CFG_PLL_TYPE_SHIFT	(0)
#define PLL_16FFT_CFG_PLL_TYPE_MASK	(0x3 << 0)
#define PLL_16FFT_CFG_PLL_TYPE_FRACF	1

/* CAL CTRL register bits */
#define PLL_16FFT_CAL_CTRL_CAL_EN               BIT(31)
#define PLL_16FFT_CAL_CTRL_FAST_CAL             BIT(20)
#define PLL_16FFT_CAL_CTRL_CAL_BYP              BIT(15)
#define PLL_16FFT_CAL_CTRL_CAL_CNT_SHIFT        16
#define PLL_16FFT_CAL_CTRL_CAL_CNT_MASK         (0x7 << 16)

/* CTRL register bits */
#define PLL_16FFT_CTRL_BYPASS_EN	BIT(31)
#define PLL_16FFT_CTRL_PLL_EN		BIT(15)
#define PLL_16FFT_CTRL_DSM_EN		BIT(1)

/* STAT register bits */
#define PLL_16FFT_STAT_LOCK		BIT(0)

/* FREQ_CTRL0 bits */
#define PLL_16FFT_FREQ_CTRL0_FB_DIV_INT_MASK	0xfff

/* DIV CTRL register bits */
#define PLL_16FFT_DIV_CTRL_REF_DIV_MASK		0x3f

/* HSDIV register bits*/
#define PLL_16FFT_HSDIV_CTRL_CLKOUT_EN          BIT(15)

/* FREQ_CTRL1 bits */
#define PLL_16FFT_FREQ_CTRL1_FB_DIV_FRAC_BITS	24
#define PLL_16FFT_FREQ_CTRL1_FB_DIV_FRAC_MASK	0xffffff
#define PLL_16FFT_FREQ_CTRL1_FB_DIV_FRAC_SHIFT	0

/* KICK register magic values */
#define PLL_KICK0_VALUE				0x68ef3490
#define PLL_KICK1_VALUE				0xd172bc5a

/**
 * struct ti_pll_clk - TI PLL clock data info structure
 * @clk: core clock structure
 * @reg: memory address of the PLL controller
 */
struct ti_pll_clk {
	struct clk_hw hw;
	void __iomem *reg;
	const char *parent;
};

static inline struct ti_pll_clk *to_pll(struct clk_hw *hw)
{
	return container_of(hw, struct ti_pll_clk, hw);
}

static int ti_pll_wait_for_lock(struct ti_pll_clk *pll)
{
	u32 stat;
	u32 cfg;
	u32 cal;
	u32 freq_ctrl1;
	int i;
	u32 pllfm;
	u32 pll_type;
	int success;

	for (i = 0; i < 100000; i++) {
		stat = readl(pll->reg + PLL_16FFT_STAT);
		if (stat & PLL_16FFT_STAT_LOCK) {
			success = 1;
			break;
		}
	}

	/* Enable calibration if not in fractional mode of the FRACF PLL */
	freq_ctrl1 = readl(pll->reg + PLL_16FFT_FREQ_CTRL1);
	pllfm = freq_ctrl1 & PLL_16FFT_FREQ_CTRL1_FB_DIV_FRAC_MASK;
	pllfm >>= PLL_16FFT_FREQ_CTRL1_FB_DIV_FRAC_SHIFT;
	cfg = readl(pll->reg + PLL_16FFT_CFG);
	pll_type = (cfg & PLL_16FFT_CFG_PLL_TYPE_MASK) >> PLL_16FFT_CFG_PLL_TYPE_SHIFT;

	if (success && pll_type == PLL_16FFT_CFG_PLL_TYPE_FRACF && pllfm == 0) {
		cal = readl(pll->reg + PLL_16FFT_CAL_CTRL);

		/* Enable calibration for FRACF */
		cal |= PLL_16FFT_CAL_CTRL_CAL_EN;

		/* Enable fast cal mode */
		cal |= PLL_16FFT_CAL_CTRL_FAST_CAL;

		/* Disable calibration bypass */
		cal &= ~PLL_16FFT_CAL_CTRL_CAL_BYP;

		/* Set CALCNT to 2 */
		cal &= ~PLL_16FFT_CAL_CTRL_CAL_CNT_MASK;
		cal |= 2 << PLL_16FFT_CAL_CTRL_CAL_CNT_SHIFT;

		/* Note this register does not readback the written value. */
		writel(cal, pll->reg + PLL_16FFT_CAL_CTRL);

		success = 0;
		for (i = 0; i < 100000; i++) {
			stat = readl(pll->reg + PLL_16FFT_CAL_STAT);
			if (stat & PLL_16FFT_CAL_STAT_CAL_LOCK) {
				success = 1;
				break;
			}
		}
	}

	if (success == 0) {
		pr_err("%s: pll (%s) failed to lock\n", __func__,
		       pll->hw.clk.name);
		return -EBUSY;
	} else {
		return 0;
	}
}

static unsigned long ti_pll_clk_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct ti_pll_clk *pll = to_pll(hw);
	u64 current_freq;
	u64 parent_freq = parent_rate;
	u32 pllm;
	u32 plld;
	u32 pllfm;
	u32 ctrl;

	/* Check if we are in bypass */
	ctrl = readl(pll->reg + PLL_16FFT_CTRL);
	if (ctrl & PLL_16FFT_CTRL_BYPASS_EN)
		return parent_freq;

	pllm = readl(pll->reg + PLL_16FFT_FREQ_CTRL0);
	pllfm = readl(pll->reg + PLL_16FFT_FREQ_CTRL1);

	plld = readl(pll->reg + PLL_16FFT_DIV_CTRL) &
		PLL_16FFT_DIV_CTRL_REF_DIV_MASK;

	current_freq = parent_freq * pllm;

	do_div(current_freq, plld);

	if (pllfm) {
		u64 tmp;

		tmp = parent_freq * pllfm;
		do_div(tmp, plld);
		tmp >>= PLL_16FFT_FREQ_CTRL1_FB_DIV_FRAC_BITS;
		current_freq += tmp;
	}

	return current_freq;
}

static int ti_pll_clk_set_rate(struct ti_pll_clk *pll, unsigned long rate,
			       unsigned long parent_rate)
{
	u64 parent_freq = parent_rate;
	int ret;
	u32 ctrl;
	unsigned long pllm;
	u32 pllfm = 0;
	unsigned long plld;
	u32 div_ctrl;
	u32 rem;
	int shift;

	if (rate != parent_freq)
		/*
		 * Attempt with higher max multiplier value first to give
		 * some space for fractional divider to kick in.
		 */
		for (shift = 8; shift >= 0; shift -= 8) {
			rational_best_approximation(rate, parent_freq,
				((PLL_16FFT_FREQ_CTRL0_FB_DIV_INT_MASK + 1) << shift) - 1,
				PLL_16FFT_DIV_CTRL_REF_DIV_MASK, &pllm, &plld);
			if (pllm / plld <= PLL_16FFT_FREQ_CTRL0_FB_DIV_INT_MASK)
				break;
		}

	/* Put PLL to bypass mode */
	ctrl = readl(pll->reg + PLL_16FFT_CTRL);
	ctrl |= PLL_16FFT_CTRL_BYPASS_EN;
	writel(ctrl, pll->reg + PLL_16FFT_CTRL);

	if (rate == parent_freq)
		return 0;

	pr_debug("%s: pre-frac-calc: rate=%u, parent_freq=%u, plld=%u, pllm=%u\n",
	      __func__, (u32)rate, (u32)parent_freq, (u32)plld, (u32)pllm);

	/* Check if we need fractional config */
	if (plld > 1) {
		pllfm = pllm % plld;
		pllfm <<= PLL_16FFT_FREQ_CTRL1_FB_DIV_FRAC_BITS;
		rem = pllfm % plld;
		pllfm /= plld;
		if (rem)
			pllfm++;
		pllm /= plld;
		plld = 1;
	}

	if (pllfm)
		ctrl |= PLL_16FFT_CTRL_DSM_EN;
	else
		ctrl &= ~PLL_16FFT_CTRL_DSM_EN;

	writel(pllm, pll->reg + PLL_16FFT_FREQ_CTRL0);
	writel(pllfm, pll->reg + PLL_16FFT_FREQ_CTRL1);

	/*
	 * div_ctrl register contains other divider values, so rmw
	 * only plld and leave existing values alone
	 */
	div_ctrl = readl(pll->reg + PLL_16FFT_DIV_CTRL);
	div_ctrl &= ~PLL_16FFT_DIV_CTRL_REF_DIV_MASK;
	div_ctrl |= plld;
	writel(div_ctrl, pll->reg + PLL_16FFT_DIV_CTRL);

	ctrl &= ~PLL_16FFT_CTRL_BYPASS_EN;
	ctrl |= PLL_16FFT_CTRL_PLL_EN;
	writel(ctrl, pll->reg + PLL_16FFT_CTRL);

	ret = ti_pll_wait_for_lock(pll);
	if (ret)
		return ret;

	pr_debug("%s: pllm=%u, plld=%u, pllfm=%u, parent_freq=%u\n",
		 __func__, (u32)pllm, (u32)plld, (u32)pllfm, (u32)parent_freq);

	return 0;
}

static int __ti_pll_clk_set_rate(struct clk_hw *hw, unsigned long rate,
				 unsigned long parent_rate)
{
	struct ti_pll_clk *pll = to_pll(hw);

	return ti_pll_clk_set_rate(pll, rate, parent_rate);
}

static long ti_pll_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *prate)
{
	return rate; /* FIXME */
}

static int ti_pll_clk_enable(struct clk_hw *hw)
{
	struct ti_pll_clk *pll = to_pll(hw);
	u32 ctrl;

	ctrl = readl(pll->reg + PLL_16FFT_CTRL);
	ctrl &= ~PLL_16FFT_CTRL_BYPASS_EN;
	ctrl |= PLL_16FFT_CTRL_PLL_EN;
	writel(ctrl, pll->reg + PLL_16FFT_CTRL);

	return ti_pll_wait_for_lock(pll);
}

static void ti_pll_clk_disable(struct clk_hw *hw)
{
	struct ti_pll_clk *pll = to_pll(hw);
	u32 ctrl;

	ctrl = readl(pll->reg + PLL_16FFT_CTRL);
	ctrl |= PLL_16FFT_CTRL_BYPASS_EN;
	writel(ctrl, pll->reg + PLL_16FFT_CTRL);
}

static const struct clk_ops ti_pll_clk_ops = {
	.recalc_rate = ti_pll_clk_recalc_rate,
	.round_rate = ti_pll_clk_round_rate,
	.set_rate = __ti_pll_clk_set_rate,
	.enable = ti_pll_clk_enable,
	.disable = ti_pll_clk_disable,
};

void ti_k3_pll_init(void __iomem *reg)
{
	u32 cfg, ctrl, hsdiv_presence_bit, hsdiv_ctrl_offs;
	int i;

	/* Unlock the PLL registers */
	writel(PLL_KICK0_VALUE, reg + PLL_KICK0);
	writel(PLL_KICK1_VALUE, reg + PLL_KICK1);

	/* Enable all HSDIV outputs */
	cfg = readl(reg + PLL_16FFT_CFG);
	for (i = 0; i < 16; i++) {
		hsdiv_presence_bit = BIT(16 + i);
		hsdiv_ctrl_offs = 0x80 + (i * 4);
		/* Enable HSDIV output if present */
		if ((hsdiv_presence_bit & cfg) != 0UL) {
			ctrl = readl(reg + hsdiv_ctrl_offs);
			ctrl |= PLL_16FFT_HSDIV_CTRL_CLKOUT_EN;
			writel(ctrl, reg + hsdiv_ctrl_offs);
		}
	}
}

int ti_k3_pll_set_rate(void __iomem *reg, unsigned long rate, unsigned long parent_rate)
{
	struct ti_pll_clk pll = {
		.reg = reg,
		.hw.clk.name = "PBL",
	};

	return ti_pll_clk_set_rate(&pll, rate, parent_rate);
}

struct clk *clk_register_ti_k3_pll(const char *name, const char *parent_name,
				   void __iomem *reg)
{
	struct ti_pll_clk *pll;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	pll->reg = reg;

	pll->parent = parent_name;
	pll->hw.clk.ops = &ti_pll_clk_ops;
	pll->hw.clk.name = name;
	pll->hw.clk.parent_names = &pll->parent;
	pll->hw.clk.num_parents = 1;

	ti_k3_pll_init(pll->reg);

	ret = bclk_register(&pll->hw.clk);
	if (ret) {
		kfree(pll);
		return ERR_PTR(ret);
	}

	return &pll->hw.clk;
}

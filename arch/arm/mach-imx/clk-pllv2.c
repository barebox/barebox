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

/* PLL Register Offsets */
#define MXC_PLL_DP_CTL			0x00
#define MXC_PLL_DP_CONFIG		0x04
#define MXC_PLL_DP_OP			0x08
#define MXC_PLL_DP_MFD			0x0C
#define MXC_PLL_DP_MFN			0x10
#define MXC_PLL_DP_MFNMINUS		0x14
#define MXC_PLL_DP_MFNPLUS		0x18
#define MXC_PLL_DP_HFS_OP		0x1C
#define MXC_PLL_DP_HFS_MFD		0x20
#define MXC_PLL_DP_HFS_MFN		0x24
#define MXC_PLL_DP_MFN_TOGC		0x28
#define MXC_PLL_DP_DESTAT		0x2c

/* PLL Register Bit definitions */
#define MXC_PLL_DP_CTL_MUL_CTRL		0x2000
#define MXC_PLL_DP_CTL_DPDCK0_2_EN	0x1000
#define MXC_PLL_DP_CTL_DPDCK0_2_OFFSET	12
#define MXC_PLL_DP_CTL_ADE		0x800
#define MXC_PLL_DP_CTL_REF_CLK_DIV	0x400
#define MXC_PLL_DP_CTL_REF_CLK_SEL_MASK	(3 << 8)
#define MXC_PLL_DP_CTL_REF_CLK_SEL_OFFSET	8
#define MXC_PLL_DP_CTL_HFSM		0x80
#define MXC_PLL_DP_CTL_PRE		0x40
#define MXC_PLL_DP_CTL_UPEN		0x20
#define MXC_PLL_DP_CTL_RST		0x10
#define MXC_PLL_DP_CTL_RCP		0x8
#define MXC_PLL_DP_CTL_PLM		0x4
#define MXC_PLL_DP_CTL_BRM0		0x2
#define MXC_PLL_DP_CTL_LRF		0x1

#define MXC_PLL_DP_CONFIG_BIST		0x8
#define MXC_PLL_DP_CONFIG_SJC_CE	0x4
#define MXC_PLL_DP_CONFIG_AREN		0x2
#define MXC_PLL_DP_CONFIG_LDREQ		0x1

#define MXC_PLL_DP_OP_MFI_OFFSET	4
#define MXC_PLL_DP_OP_MFI_MASK		(0xF << 4)
#define MXC_PLL_DP_OP_PDF_OFFSET	0
#define MXC_PLL_DP_OP_PDF_MASK		0xF

#define MXC_PLL_DP_MFD_OFFSET		0
#define MXC_PLL_DP_MFD_MASK		0x07FFFFFF

#define MXC_PLL_DP_MFN_OFFSET		0x0
#define MXC_PLL_DP_MFN_MASK		0x07FFFFFF

#define MXC_PLL_DP_MFN_TOGC_TOG_DIS	(1 << 17)
#define MXC_PLL_DP_MFN_TOGC_TOG_EN	(1 << 16)
#define MXC_PLL_DP_MFN_TOGC_CNT_OFFSET	0x0
#define MXC_PLL_DP_MFN_TOGC_CNT_MASK	0xFFFF

#define MXC_PLL_DP_DESTAT_TOG_SEL	(1 << 31)
#define MXC_PLL_DP_DESTAT_MFN		0x07FFFFFF

#define MAX_DPLL_WAIT_TRIES	1000 /* 1000 * udelay(1) = 1ms */

struct clk_pllv2 {
	struct clk clk;
	void __iomem *reg;
	const char *parent;
};

static unsigned long __clk_pllv2_recalc_rate(unsigned long parent_rate,
		u32 dp_ctl, u32 dp_op, u32 dp_mfd, u32 dp_mfn)
{
	long mfi, mfn, mfd, pdf, ref_clk, mfn_abs;
	unsigned long dbl;
	uint64_t temp;

	dbl = dp_ctl & MXC_PLL_DP_CTL_DPDCK0_2_EN;

	pdf = dp_op & MXC_PLL_DP_OP_PDF_MASK;
	mfi = (dp_op & MXC_PLL_DP_OP_MFI_MASK) >> MXC_PLL_DP_OP_MFI_OFFSET;
	mfi = (mfi <= 5) ? 5 : mfi;
	mfd = dp_mfd & MXC_PLL_DP_MFD_MASK;
	mfn = mfn_abs = dp_mfn & MXC_PLL_DP_MFN_MASK;
	/* Sign extend to 32-bits */
	if (mfn >= 0x04000000) {
		mfn |= 0xFC000000;
		mfn_abs = -mfn;
	}

	ref_clk = 2 * parent_rate;
	if (dbl != 0)
		ref_clk *= 2;

	ref_clk /= (pdf + 1);
	temp = (u64) ref_clk * mfn_abs;
	do_div(temp, mfd + 1);
	if (mfn < 0)
		temp = -temp;
	temp = (ref_clk * mfi) + temp;

	return temp;
}

static unsigned long clk_pllv2_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	u32 dp_op, dp_mfd, dp_mfn, dp_ctl;
	void __iomem *pllbase;
	struct clk_pllv2 *pll = container_of(clk, struct clk_pllv2, clk);

	pllbase = pll->reg;

	dp_ctl = __raw_readl(pllbase + MXC_PLL_DP_CTL);
	dp_op = __raw_readl(pllbase + MXC_PLL_DP_OP);
	dp_mfd = __raw_readl(pllbase + MXC_PLL_DP_MFD);
	dp_mfn = __raw_readl(pllbase + MXC_PLL_DP_MFN);

	return __clk_pllv2_recalc_rate(parent_rate, dp_ctl, dp_op, dp_mfd, dp_mfn);
}

struct clk_ops clk_pllv2_ops = {
	.recalc_rate = clk_pllv2_recalc_rate,
};

struct clk *imx_clk_pllv2(const char *name, const char *parent,
		void __iomem *base)
{
	struct clk_pllv2 *pll = xzalloc(sizeof(*pll));
	int ret;

	pll->parent = parent;
	pll->reg = base;
	pll->clk.ops = &clk_pllv2_ops;
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

/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <clock.h>
#include <io.h>
#include <malloc.h>
#include <asm-generic/div64.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <mach/iomap.h>

#include "clk.h"

#define PLL_BASE_BYPASS		BIT(31)
#define PLL_BASE_ENABLE		BIT(30)
#define PLL_BASE_REF_ENABLE	BIT(29)
#define PLL_BASE_OVERRIDE	BIT(28)

#define PLL_BASE_DIVP_SHIFT	20
#define PLL_BASE_DIVP_WIDTH	3
#define PLL_BASE_DIVN_SHIFT	8
#define PLL_BASE_DIVN_WIDTH	10
#define PLL_BASE_DIVM_SHIFT	0
#define PLL_BASE_DIVM_WIDTH	5
#define PLLU_POST_DIVP_MASK	0x1

#define PLL_MISC_DCCON_SHIFT	20
#define PLL_MISC_CPCON_SHIFT	8
#define PLL_MISC_CPCON_WIDTH	4
#define PLL_MISC_CPCON_MASK	((1 << PLL_MISC_CPCON_WIDTH) - 1)
#define PLL_MISC_LFCON_SHIFT	4
#define PLL_MISC_LFCON_WIDTH	4
#define PLL_MISC_LFCON_MASK	((1 << PLL_MISC_LFCON_WIDTH) - 1)
#define PLL_MISC_VCOCON_SHIFT	0
#define PLL_MISC_VCOCON_WIDTH	4
#define PLL_MISC_VCOCON_MASK	((1 << PLL_MISC_VCOCON_WIDTH) - 1)

#define OUT_OF_TABLE_CPCON	8

#define PMC_PLLP_WB0_OVERRIDE			0xf8
#define PMC_PLLP_WB0_OVERRIDE_PLLM_ENABLE	BIT(12)
#define PMC_PLLP_WB0_OVERRIDE_PLLM_OVERRIDE	BIT(11)

#define PLL_POST_LOCK_DELAY	50

#define PLLDU_LFCON_SET_DIVN	600

#define PLLE_BASE_DIVCML_SHIFT	24
#define PLLE_BASE_DIVCML_MASK	0xf
#define PLLE_BASE_DIVCML_WIDTH	4
#define PLLE_BASE_DIVP_SHIFT	16
#define PLLE_BASE_DIVP_WIDTH	7
#define PLLE_BASE_DIVN_SHIFT	8
#define PLLE_BASE_DIVN_WIDTH	8
#define PLLE_BASE_DIVM_SHIFT	0
#define PLLE_BASE_DIVM_WIDTH	8

#define PLLE_MISC_SETUP_BASE_SHIFT	16
#define PLLE_MISC_SETUP_BASE_MASK	(0xffff << PLLE_MISC_SETUP_BASE_SHIFT)
#define PLLE_MISC_LOCK_ENABLE		BIT(9)
#define PLLE_MISC_READY			BIT(15)
#define PLLE_MISC_SETUP_EX_SHIFT	2
#define PLLE_MISC_SETUP_EX_MASK		(3 << PLLE_MISC_SETUP_EX_SHIFT)
#define PLLE_MISC_SETUP_MASK		(PLLE_MISC_SETUP_BASE_MASK |	\
					 PLLE_MISC_SETUP_EX_MASK)
#define PLLE_MISC_SETUP_VALUE		(7 << PLLE_MISC_SETUP_BASE_SHIFT)

#define XUSBIO_PLL_CFG0				0x51c
#define XUSBIO_PLL_CFG0_PADPLL_RESET_SWCTL	(1 << 0)
#define XUSBIO_PLL_CFG0_CLK_ENABLE_SWCTL	(1 << 2)
#define XUSBIO_PLL_CFG0_PADPLL_USE_LOCKDET	(1 << 6)
#define XUSBIO_PLL_CFG0_SEQ_ENABLE		(1 << 24)
#define XUSBIO_PLL_CFG0_SEQ_START_STATE		(1 << 25)

#define PLLE_SS_CTRL			0x68
#define PLLE_SS_CNTL_BYPASS_SS		(7 << 10)
#define PLLE_SS_CNTL_INTERP_RESET	(1 << 11)
#define PLLE_SS_CNTL_SSC_BYP		(1 << 12)
#define PLLE_SS_CNTL_CENTER		(1 << 14)
#define PLLE_SS_CNTL_INVERT		(1 << 15)
#define PLLE_SS_MAX_MASK		0x1ff
#define PLLE_SS_MAX_VAL			0x25
#define PLLE_SS_INC_MASK		(0xff << 16)
#define PLLE_SS_INC_VAL			(0x1 << 16)
#define PLLE_SS_INCINTRV_MASK		(0x3f << 24)
#define PLLE_SS_INCINTRV_VAL		(0x20 << 24)
#define PLLE_SS_COEFFICIENTS_MASK \
	(PLLE_SS_MAX_MASK | PLLE_SS_INC_MASK | PLLE_SS_INCINTRV_MASK)
#define PLLE_SS_COEFFICIENTS_VAL \
	(PLLE_SS_MAX_VAL | PLLE_SS_INC_VAL | PLLE_SS_INCINTRV_VAL)

#define PLLE_MISC_VREG_CTRL_SHIFT	2
#define PLLE_MISC_VREG_CTRL_MASK	(2 << PLLE_MISC_VREG_CTRL_SHIFT)
#define PLLE_MISC_VREG_BG_CTRL_SHIFT	4
#define PLLE_MISC_VREG_BG_CTRL_MASK	(3 << PLLE_MISC_VREG_BG_CTRL_SHIFT)
#define PLLE_MISC_PLLE_PTS		(1 << 8)
#define PLLE_MISC_IDDQ_SW_VALUE		(1 << 13)
#define PLLE_MISC_IDDQ_SW_CTRL		(1 << 14)

#define PLLE_AUX_PLLP_SEL		(1 << 2)
#define PLLE_AUX_USE_LOCKDET		(1 << 3)
#define PLLE_AUX_ENABLE_SWCTL		(1 << 4)
#define PLLE_AUX_SS_SWCTL		(1 << 6)
#define PLLE_AUX_SEQ_ENABLE		(1 << 24)
#define PLLE_AUX_SEQ_START_STATE	(1 << 25)
#define PLLE_AUX_PLLRE_SEL		(1 << 28)

#define PMC_SATA_PWRGT			0x1ac
#define PMC_SATA_PWRGT_PLLE_IDDQ_VALUE	BIT(5)
#define PMC_SATA_PWRGT_PLLE_IDDQ_SWCTL	BIT(4)

#define pll_readl(offset, p) readl(p->clk_base + offset)
#define pll_readl_base(p) pll_readl(p->params->base_reg, p)
#define pll_readl_misc(p) pll_readl(p->params->misc_reg, p)

#define pll_writel(val, offset, p) writel(val, p->clk_base + offset)
#define pll_writel_base(val, p) pll_writel(val, p->params->base_reg, p)
#define pll_writel_misc(val, p) pll_writel(val, p->params->misc_reg, p)

#define mask(w) ((1 << (w)) - 1)
#define divm_mask(p) mask(p->divm_width)
#define divn_mask(p) mask(p->divn_width)
#define divp_mask(p) (p->flags & TEGRA_PLLU ? PLLU_POST_DIVP_MASK :	\
		      mask(p->divp_width))

#define divm_shift(p) (p)->divm_shift
#define divn_shift(p) (p)->divn_shift
#define divp_shift(p) (p)->divp_shift

#define divm_mask_shifted(p) (divm_mask(p) << divm_shift(p))
#define divn_mask_shifted(p) (divn_mask(p) << divn_shift(p))
#define divp_mask_shifted(p) (divp_mask(p) << divp_shift(p))

#define divm_max(p) (divm_mask(p))
#define divn_max(p) (divn_mask(p))
#define divp_max(p) (1 << (divp_mask(p)))


#define to_clk_pll(_hw) container_of(_hw, struct tegra_clk_pll, hw)

static int clk_pll_is_enabled(struct clk *hw)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	u32 val;

	val = pll_readl_base(pll);

	return val & PLL_BASE_ENABLE ? 1 : 0;
}

static void clk_pll_enable_lock(struct tegra_clk_pll *pll)
{
	u32 val;

	if (!(pll->flags & TEGRA_PLL_USE_LOCK))
		return;

	val = pll_readl_misc(pll);
	val |= BIT(pll->params->lock_enable_bit_idx);
	pll_writel_misc(val, pll);
}

static int clk_pll_wait_for_lock(struct tegra_clk_pll *pll,
				 void __iomem *lock_addr, u32 lock_bit_idx)
{
	int i;
	u32 val;

	if (!(pll->flags & TEGRA_PLL_USE_LOCK)) {
		udelay(pll->params->lock_delay);
		return 0;
	}

	for (i = 0; i < pll->params->lock_delay; i++) {
		val = readl(lock_addr);
		if (val & BIT(lock_bit_idx)) {
			udelay(PLL_POST_LOCK_DELAY);
			return 0;
		}
		udelay(2); /* timeout = 2 * lock time */
	}

	pr_err("%s: Timed out waiting for pll %s lock\n", __func__,
	       pll->hw.name);

	return -1;
}

static int clk_pll_enable(struct clk *hw)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	u32 val;

	clk_pll_enable_lock(pll);

	val = pll_readl_base(pll);
	val &= ~PLL_BASE_BYPASS;
	val |= PLL_BASE_ENABLE;
	pll_writel_base(val, pll);

	clk_pll_wait_for_lock(pll, pll->clk_base + pll->params->base_reg,
			      pll->params->lock_bit_idx);

	return 0;
}

static void clk_pll_disable(struct clk *hw)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	u32 val;

	val = pll_readl_base(pll);
	val &= ~(PLL_BASE_BYPASS | PLL_BASE_ENABLE);
	pll_writel_base(val, pll);
}

static int _get_table_rate(struct clk *hw,
			   struct tegra_clk_pll_freq_table *cfg,
			   unsigned long rate, unsigned long parent_rate)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	struct tegra_clk_pll_freq_table *sel;

	for (sel = pll->freq_table; sel->input_rate != 0; sel++)
		if (sel->input_rate == parent_rate &&
		    sel->output_rate == rate)
			break;

	if (sel->input_rate == 0)
		return -EINVAL;

	cfg->input_rate = sel->input_rate;
	cfg->output_rate = sel->output_rate;
	cfg->m = sel->m;
	cfg->n = sel->n;
	cfg->p = sel->p;
	cfg->cpcon = sel->cpcon;

	return 0;
}

static unsigned long clk_pll_recalc_rate(struct clk *hw,
					 unsigned long parent_rate)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	u32 val = pll_readl_base(pll);
	u32 divn = 0, divm = 0, divp = 0;
	u64 rate = parent_rate;

	if (val & PLL_BASE_BYPASS)
		return parent_rate;

	if ((pll->flags & TEGRA_PLL_FIXED) && !(val & PLL_BASE_OVERRIDE)) {
		struct tegra_clk_pll_freq_table sel;
		if (_get_table_rate(hw, &sel, pll->fixed_rate, parent_rate)) {
			pr_err("Clock %s has unknown fixed frequency\n",
			       hw->name);
			BUG();
		}
		return pll->fixed_rate;
	}

	divp = (val >> pll->divp_shift) & (divp_mask(pll));
	if (pll->flags & TEGRA_PLLU)
		divp ^= 1;

	divn = (val >> pll->divn_shift) & (divn_mask(pll));
	divm = (val >> pll->divm_shift) & (divm_mask(pll));
	divm *= (1 << divp);

	rate *= divn;
	do_div(rate, divm);

	return rate;
}

static int _calc_rate(struct clk *hw, struct tegra_clk_pll_freq_table *cfg,
		      unsigned long rate, unsigned long parent_rate)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	unsigned long cfreq;
	u32 p_div = 0;

	switch (parent_rate) {
	case 12000000:
	case 26000000:
		cfreq = (rate <= 1000000 * 1000) ? 1000000 : 2000000;
		break;
	case 13000000:
		cfreq = (rate <= 1000000 * 1000) ? 1000000 : 2600000;
		break;
	case 16800000:
	case 19200000:
		cfreq = (rate <= 1200000 * 1000) ? 1200000 : 2400000;
		break;
	case 9600000:
	case 28800000:
		/*
		 * PLL_P_OUT1 rate is not listed in PLLA table
		 */
		cfreq = parent_rate/(parent_rate/1000000);
		break;
	default:
		pr_err("%s Unexpected reference rate %lu\n",
		       __func__, parent_rate);
		BUG();
	}

	/* Raise VCO to guarantee 0.5% accuracy */
	for (cfg->output_rate = rate; cfg->output_rate < 200 * cfreq;
	     cfg->output_rate <<= 1)
		p_div++;

	cfg->p = 1 << p_div;
	cfg->m = parent_rate / cfreq;
	cfg->n = cfg->output_rate / cfreq;
	cfg->cpcon = OUT_OF_TABLE_CPCON;

	if (cfg->m > divm_max(pll) || cfg->n > divn_max(pll) ||
	    cfg->p > divp_max(pll) || cfg->output_rate > pll->params->vco_max) {
		pr_err("%s: Failed to set %s rate %lu\n",
		       __func__, hw->name, rate);
		return -EINVAL;
	}

	return 0;
}

static long clk_pll_round_rate(struct clk *hw, unsigned long rate,
			unsigned long *prate)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	struct tegra_clk_pll_freq_table cfg;
	u64 output_rate = *prate;

	if (pll->flags & TEGRA_PLL_FIXED)
		return pll->fixed_rate;

	/* PLLM is used for memory; we do not change rate */
	if (pll->flags & TEGRA_PLLM)
		return clk_get_rate(hw);

	if (_get_table_rate(hw, &cfg, rate, *prate) &&
	    _calc_rate(hw, &cfg, rate, *prate))
		return -EINVAL;

	output_rate *= cfg.n;
	do_div(output_rate, cfg.m * cfg.p);

	return output_rate;
}

static int _program_pll(struct clk *hw, struct tegra_clk_pll_freq_table *cfg,
			unsigned long rate)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	u32 divp, val, old_base;
	int state;

	divp = __ffs(cfg->p);

	if (pll->flags & TEGRA_PLLU)
		divp ^= 1;

	old_base = val = pll_readl_base(pll);
	val &= ~((divm_mask(pll) << pll->divm_shift) |
		 (divn_mask(pll) << pll->divn_shift) |
		 (divp_mask(pll) << pll->divp_shift));
	val |= ((cfg->m << pll->divm_shift) |
		(cfg->n << pll->divn_shift) |
		(divp << pll->divp_shift));
	if (pll->flags & TEGRA_PLL_FIXED)
		val |= PLL_BASE_OVERRIDE;
	if (val == old_base) {
		return 0;
	}

	state = clk_pll_is_enabled(hw);

	if (state) {
		clk_pll_disable(hw);
		val &= ~(PLL_BASE_BYPASS | PLL_BASE_ENABLE);
	}
	pll_writel_base(val, pll);

	if (pll->flags & TEGRA_PLL_HAS_CPCON) {
		val = pll_readl_misc(pll);
		val &= ~(PLL_MISC_CPCON_MASK << PLL_MISC_CPCON_SHIFT);
		val |= cfg->cpcon << PLL_MISC_CPCON_SHIFT;
		if (pll->flags & TEGRA_PLL_SET_LFCON) {
			val &= ~(PLL_MISC_LFCON_MASK << PLL_MISC_LFCON_SHIFT);
			if (cfg->n >= PLLDU_LFCON_SET_DIVN)
				val |= 0x1 << PLL_MISC_LFCON_SHIFT;
		} else if (pll->flags & TEGRA_PLL_SET_DCCON) {
			val &= ~(0x1 << PLL_MISC_DCCON_SHIFT);
			if (rate >= (pll->params->vco_max >> 1))
				val |= 0x1 << PLL_MISC_DCCON_SHIFT;
		}
		pll_writel_misc(val, pll);
	}

	if (state)
		clk_pll_enable(hw);

	return 0;
}

static int clk_pll_set_rate(struct clk *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct tegra_clk_pll_freq_table cfg;

	if (_get_table_rate(hw, &cfg, rate, parent_rate) &&
	    _calc_rate(hw, &cfg, rate, parent_rate))
		return -EINVAL;

	return _program_pll(hw, &cfg, rate);
}

const struct clk_ops tegra_clk_pll_ops = {
	.is_enabled = clk_pll_is_enabled,
	.enable = clk_pll_enable,
	.disable = clk_pll_disable,
	.recalc_rate = clk_pll_recalc_rate,
	.round_rate = clk_pll_round_rate,
	.set_rate = clk_pll_set_rate,
};

static unsigned long clk_plle_recalc_rate(struct clk *hw,
					 unsigned long parent_rate)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	u32 val = pll_readl_base(pll);
	u32 divn = 0, divm = 0, divp = 0;
	u64 rate = parent_rate;

	divp = (val >> 16) & 0x3f;
	divn = (val >> 8) & (0xff);
	divm = (val >> 0) & (0xff);
	divm *= divp;

	rate *= divn;
	do_div(rate, divm);
	return rate;
}

static int clk_plle_training(struct tegra_clk_pll *pll)
{
	u32 val;

	/*
	 * PLLE is already disabled, and setup cleared;
	 * create falling edge on PLLE IDDQ input.
	 */
	val = readl(TEGRA_PMC_BASE + PMC_SATA_PWRGT);
	val |= PMC_SATA_PWRGT_PLLE_IDDQ_VALUE;
	writel(val, TEGRA_PMC_BASE + PMC_SATA_PWRGT);

	val = readl(TEGRA_PMC_BASE + PMC_SATA_PWRGT);
	val |= PMC_SATA_PWRGT_PLLE_IDDQ_SWCTL;
	writel(val, TEGRA_PMC_BASE + PMC_SATA_PWRGT);

	val = readl(TEGRA_PMC_BASE + PMC_SATA_PWRGT);
	val &= ~PMC_SATA_PWRGT_PLLE_IDDQ_VALUE;
	writel(val, TEGRA_PMC_BASE + PMC_SATA_PWRGT);

	return wait_on_timeout(100 * MSECOND,
			(pll_readl_misc(pll) & PLLE_MISC_READY));
}

static int clk_plle_enable(struct clk *hw)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	unsigned long input_rate = clk_get_rate(clk_get_parent(hw));
	struct tegra_clk_pll_freq_table sel;
	u32 val;
	int err;

	if (_get_table_rate(hw, &sel, pll->fixed_rate, input_rate))
		return -EINVAL;

	clk_pll_disable(hw);

	val = pll_readl_misc(pll);
	val &= ~(PLLE_MISC_LOCK_ENABLE | PLLE_MISC_SETUP_MASK);
	pll_writel_misc(val, pll);

	val = pll_readl_misc(pll);
	if (!(val & PLLE_MISC_READY)) {
		err = clk_plle_training(pll);
		if (err)
			return err;
	}

	/* configure dividers */
	val = pll_readl_base(pll);
	val &= ~(0x3fffff);
	val &= ~(PLLE_BASE_DIVCML_WIDTH << PLLE_BASE_DIVCML_SHIFT);
	val |= sel.m << 0;
	val |= sel.n << 8;
	val |= sel.p << 16;
	val |= sel.cpcon << PLLE_BASE_DIVCML_SHIFT;
	pll_writel_base(val, pll);

	val = pll_readl_misc(pll);
	val |= PLLE_MISC_SETUP_VALUE;
	val |= PLLE_MISC_LOCK_ENABLE;
	pll_writel_misc(val, pll);

	val = readl(pll->clk_base + PLLE_SS_CTRL);
	val |= PLLE_SS_CNTL_BYPASS_SS;
	writel(val, pll->clk_base + PLLE_SS_CTRL);

	val = pll_readl_base(pll);
	val |= (PLL_BASE_BYPASS | PLL_BASE_ENABLE);
	pll_writel_base(val, pll);

	clk_pll_wait_for_lock(pll, pll->clk_base + pll->params->base_reg,
			      pll->params->lock_bit_idx);

	return 0;
}

const struct clk_ops tegra_clk_plle_ops = {
	.recalc_rate = clk_plle_recalc_rate,
	.is_enabled = clk_pll_is_enabled,
	.disable = clk_pll_disable,
	.enable = clk_plle_enable,
};

static int clk_plle_tegra114_enable(struct clk *hw)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	unsigned long input_rate = clk_get_rate(clk_get_parent(hw));
	struct tegra_clk_pll_freq_table sel;
	u32 val;
	int ret;

	if (_get_table_rate(hw, &sel, pll->fixed_rate, input_rate))
		return -EINVAL;

	val = pll_readl_base(pll);
	val &= ~BIT(29); /* Disable lock override */
	pll_writel_base(val, pll);

	val = pll_readl(pll->params->aux_reg, pll);
	val |= PLLE_AUX_ENABLE_SWCTL;
	val &= ~PLLE_AUX_SEQ_ENABLE;
	pll_writel(val, pll->params->aux_reg, pll);
	udelay(1);

	val = pll_readl_misc(pll);
	val |= PLLE_MISC_LOCK_ENABLE;
	val |= PLLE_MISC_IDDQ_SW_CTRL;
	val &= ~PLLE_MISC_IDDQ_SW_VALUE;
	val |= PLLE_MISC_PLLE_PTS;
	val |= PLLE_MISC_VREG_BG_CTRL_MASK | PLLE_MISC_VREG_CTRL_MASK;
	pll_writel_misc(val, pll);
	udelay(5);

	val = pll_readl(PLLE_SS_CTRL, pll);
	val |= PLLE_SS_CNTL_BYPASS_SS;
	pll_writel(val, PLLE_SS_CTRL, pll);

	val = pll_readl_base(pll);
	val &= ~(divp_mask_shifted(pll) | divn_mask_shifted(pll) |
		 divm_mask_shifted(pll));
	val &= ~(PLLE_BASE_DIVCML_MASK << PLLE_BASE_DIVCML_SHIFT);
	val |= sel.m << divm_shift(pll);
	val |= sel.n << divn_shift(pll);
	val |= sel.cpcon << PLLE_BASE_DIVCML_SHIFT;
	pll_writel_base(val, pll);
	udelay(1);

	clk_pll_enable(hw);
	ret = clk_pll_wait_for_lock(pll, pll->clk_base + pll->params->misc_reg,
				    pll->params->lock_bit_idx);

	if (ret < 0)
		return ret;

	val = pll_readl(PLLE_SS_CTRL, pll);
	val &= ~(PLLE_SS_CNTL_CENTER | PLLE_SS_CNTL_INVERT);
	val &= ~PLLE_SS_COEFFICIENTS_MASK;
	val |= PLLE_SS_COEFFICIENTS_VAL;
	pll_writel(val, PLLE_SS_CTRL, pll);
	val &= ~(PLLE_SS_CNTL_SSC_BYP | PLLE_SS_CNTL_BYPASS_SS);
	pll_writel(val, PLLE_SS_CTRL, pll);
	udelay(1);
	val &= ~PLLE_SS_CNTL_INTERP_RESET;
	pll_writel(val, PLLE_SS_CTRL, pll);
	udelay(1);

	/* Enable hw control of xusb brick pll */
	val = pll_readl_misc(pll);
	val &= ~PLLE_MISC_IDDQ_SW_CTRL;
	pll_writel_misc(val, pll);

	val = pll_readl(pll->params->aux_reg, pll);
	val |= (PLLE_AUX_USE_LOCKDET | PLLE_AUX_SEQ_START_STATE);
	val &= ~(PLLE_AUX_ENABLE_SWCTL | PLLE_AUX_SS_SWCTL);
	pll_writel(val, pll->params->aux_reg, pll);
	udelay(1);
	val |= PLLE_AUX_SEQ_ENABLE;
	pll_writel(val, pll->params->aux_reg, pll);

	val = pll_readl(XUSBIO_PLL_CFG0, pll);
	val |= (XUSBIO_PLL_CFG0_PADPLL_USE_LOCKDET |
		XUSBIO_PLL_CFG0_SEQ_START_STATE);
	val &= ~(XUSBIO_PLL_CFG0_CLK_ENABLE_SWCTL |
		 XUSBIO_PLL_CFG0_PADPLL_RESET_SWCTL);
	pll_writel(val, XUSBIO_PLL_CFG0, pll);
	udelay(1);
	val |= XUSBIO_PLL_CFG0_SEQ_ENABLE;
	pll_writel(val, XUSBIO_PLL_CFG0, pll);

	return ret;
}

static void clk_plle_tegra114_disable(struct clk *hw)
{
	struct tegra_clk_pll *pll = to_clk_pll(hw);
	u32 val;

	clk_pll_disable(hw);

	val = pll_readl_misc(pll);
	val |= PLLE_MISC_IDDQ_SW_CTRL | PLLE_MISC_IDDQ_SW_VALUE;
	pll_writel_misc(val, pll);
	udelay(1);
}

const struct clk_ops tegra_clk_plle_tegra114_ops = {
	.recalc_rate = clk_pll_recalc_rate,
	.is_enabled = clk_pll_is_enabled,
	.disable = clk_plle_tegra114_disable,
	.enable = clk_plle_tegra114_enable,
};

static struct clk *_tegra_clk_register_pll(const char *name,
		const char *parent_name, void __iomem *clk_base,
		unsigned long flags, unsigned long fixed_rate,
		struct tegra_clk_pll_params *pll_params, u8 pll_flags,
		struct tegra_clk_pll_freq_table *freq_table,
		const struct clk_ops *ops)
{
	struct tegra_clk_pll *pll;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return NULL;

	pll->parent = parent_name;
	pll->hw.name = name;
	pll->hw.ops = ops;
	pll->hw.flags = flags;
	pll->hw.parent_names = (pll->parent ? &pll->parent : NULL);
	pll->hw.num_parents = (pll->parent ? 1 : 0);

	pll->clk_base = clk_base;

	pll->freq_table = freq_table;
	pll->params = pll_params;
	pll->fixed_rate = fixed_rate;
	pll->flags = pll_flags;

	pll->divp_shift = PLL_BASE_DIVP_SHIFT;
	pll->divp_width = PLL_BASE_DIVP_WIDTH;
	pll->divn_shift = PLL_BASE_DIVN_SHIFT;
	pll->divn_width = PLL_BASE_DIVN_WIDTH;
	pll->divm_shift = PLL_BASE_DIVM_SHIFT;
	pll->divm_width = PLL_BASE_DIVM_WIDTH;

	ret = clk_register(&pll->hw);
	if (ret) {
		kfree(pll);
		return ERR_PTR(ret);
	}

	return &pll->hw;
}

struct clk *tegra_clk_register_pll(const char *name, const char *parent_name,
		void __iomem *clk_base,
		unsigned long flags, unsigned long fixed_rate,
		struct tegra_clk_pll_params *pll_params, u8 pll_flags,
		struct tegra_clk_pll_freq_table *freq_table)
{
	return _tegra_clk_register_pll(name, parent_name, clk_base,
			flags, fixed_rate, pll_params, pll_flags, freq_table,
			&tegra_clk_pll_ops);
}

struct clk *tegra_clk_register_plle(const char *name, const char *parent_name,
		void __iomem *clk_base,
		unsigned long flags, unsigned long fixed_rate,
		struct tegra_clk_pll_params *pll_params, u8 pll_flags,
		struct tegra_clk_pll_freq_table *freq_table)
{
	return _tegra_clk_register_pll(name, parent_name, clk_base,
			flags, fixed_rate, pll_params, pll_flags, freq_table,
			&tegra_clk_plle_ops);
}

struct clk *tegra_clk_register_plle_tegra114(const char *name,
		const char *parent_name, void __iomem *clk_base,
		unsigned long flags, unsigned long fixed_rate,
		struct tegra_clk_pll_params *pll_params, u8 pll_flags,
		struct tegra_clk_pll_freq_table *freq_table)
{
	return _tegra_clk_register_pll(name, parent_name, clk_base,
			flags, fixed_rate, pll_params, pll_flags, freq_table,
			&tegra_clk_plle_tegra114_ops);
}

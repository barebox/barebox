// SPDX-License-Identifier: GPL-2.0-only
/*
 * Zynq UltraScale+ MPSoC PLL Clock
 *
 * Copyright (C) 2019 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 *
 * Based on the Linux driver in drivers/clk/zynqmp/
 *
 * Copyright (C) 2016-2018 Xilinx
 */

#include <common.h>
#include <linux/clk.h>
#include <mach/firmware-zynqmp.h>

#include "clk-zynqmp.h"

struct zynqmp_pll {
	struct clk_hw hw;
	unsigned int clk_id;
	const char *parent;
	const struct zynqmp_eemi_ops *ops;
};

#define to_zynqmp_pll(_hw) \
	container_of(_hw, struct zynqmp_pll, hw)

#define PLL_FBDIV_MIN		25
#define PLL_FBDIV_MAX		125

#define PS_PLL_VCO_MIN		1500000000
#define PS_PLL_VCO_MAX		3000000000UL

enum pll_mode {
	PLL_MODE_INT,
	PLL_MODE_FRAC,
};

#define FRAC_DIV 		(1 << 16)

static inline enum pll_mode zynqmp_pll_get_mode(struct zynqmp_pll *pll)
{
	u32 ret_payload[PAYLOAD_ARG_CNT];

	pll->ops->ioctl(0, IOCTL_GET_PLL_FRAC_MODE, pll->clk_id, 0,
			      ret_payload);

	return ret_payload[1];
}

static inline void zynqmp_pll_set_mode(struct zynqmp_pll *pll, enum pll_mode mode)
{
	pll->ops->ioctl(0, IOCTL_SET_PLL_FRAC_MODE, pll->clk_id, mode, NULL);
}

static long zynqmp_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *prate)
{
	struct zynqmp_pll *pll = to_zynqmp_pll(hw);
	u32 fbdiv;
	long rate_div;

	rate_div = (rate * FRAC_DIV) / *prate;
	if (rate_div % FRAC_DIV)
		zynqmp_pll_set_mode(pll, PLL_MODE_FRAC);
	else
		zynqmp_pll_set_mode(pll, PLL_MODE_INT);

	if (zynqmp_pll_get_mode(pll) == PLL_MODE_FRAC) {
		if (rate > PS_PLL_VCO_MAX) {
			fbdiv = rate / PS_PLL_VCO_MAX;
			rate = rate / (fbdiv + 1);
		}
		if (rate < PS_PLL_VCO_MIN) {
			fbdiv = DIV_ROUND_UP(PS_PLL_VCO_MIN, rate);
			rate = rate * fbdiv;
		}
	} else {
		fbdiv = DIV_ROUND_CLOSEST(rate, *prate);
		fbdiv = clamp_t(u32, fbdiv, PLL_FBDIV_MIN, PLL_FBDIV_MAX);
		rate = *prate * fbdiv;
	}

	return rate;
}

static unsigned long zynqmp_pll_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct zynqmp_pll *pll = to_zynqmp_pll(hw);
	u32 clk_id = pll->clk_id;
	u32 fbdiv, data;
	unsigned long rate, frac;
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = pll->ops->clock_getdivider(clk_id, &fbdiv);

	rate = parent_rate * fbdiv;

	if (zynqmp_pll_get_mode(pll) == PLL_MODE_FRAC) {
		pll->ops->ioctl(0, IOCTL_GET_PLL_FRAC_DATA, clk_id, 0,
				ret_payload);
		data = ret_payload[1];
		frac = (parent_rate * data) / FRAC_DIV;
		rate = rate + frac;
	}

	return rate;
}

static int zynqmp_pll_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct zynqmp_pll *pll = to_zynqmp_pll(hw);
	u32 clk_id = pll->clk_id;
	u32 fbdiv;
	long rate_div, frac, m, f;

	if (zynqmp_pll_get_mode(pll) == PLL_MODE_FRAC) {
		rate_div = (rate * FRAC_DIV) / parent_rate;
		m = rate_div / FRAC_DIV;
		f = rate_div % FRAC_DIV;
		m = clamp_t(u32, m, (PLL_FBDIV_MIN), (PLL_FBDIV_MAX));
		rate = parent_rate * m;
		frac = (parent_rate * f) / FRAC_DIV;

		pll->ops->clock_setdivider(clk_id, m);
		pll->ops->ioctl(0, IOCTL_SET_PLL_FRAC_DATA, clk_id, f, NULL);

		return rate + frac;
	} else {
		fbdiv = DIV_ROUND_CLOSEST(rate, parent_rate);
		fbdiv = clamp_t(u32, fbdiv, PLL_FBDIV_MIN, PLL_FBDIV_MAX);
		pll->ops->clock_setdivider(clk_id, fbdiv);

		return parent_rate * fbdiv;
	}
}

static int zynqmp_pll_is_enabled(struct clk_hw *hw)
{
	struct zynqmp_pll *pll = to_zynqmp_pll(hw);
	u32 is_enabled;
	int ret;

	ret = pll->ops->clock_getstate(pll->clk_id, &is_enabled);
	if (ret)
		return -EIO;

	return !!(is_enabled);
}

static int zynqmp_pll_enable(struct clk_hw *hw)
{
	struct zynqmp_pll *pll = to_zynqmp_pll(hw);

	if (zynqmp_pll_is_enabled(hw))
		return 0;

	return pll->ops->clock_enable(pll->clk_id);
}

static void zynqmp_pll_disable(struct clk_hw *hw)
{
	struct zynqmp_pll *pll = to_zynqmp_pll(hw);

	if (!zynqmp_pll_is_enabled(hw))
		return;

	pll->ops->clock_disable(pll->clk_id);
}

static const struct clk_ops zynqmp_pll_ops = {
	.enable = zynqmp_pll_enable,
	.disable = zynqmp_pll_disable,
	.is_enabled = zynqmp_pll_is_enabled,
	.round_rate = zynqmp_pll_round_rate,
	.recalc_rate = zynqmp_pll_recalc_rate,
	.set_rate = zynqmp_pll_set_rate,
};

struct clk *zynqmp_clk_register_pll(const char *name,
				    unsigned int clk_id,
				    const char **parents,
				    unsigned int num_parents,
				    const struct clock_topology *nodes)
{
	struct zynqmp_pll *pll;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	pll->clk_id = clk_id;
	pll->ops = zynqmp_pm_get_eemi_ops();
	pll->parent = strdup(parents[0]);

	pll->hw.clk.name = strdup(name);
	pll->hw.clk.ops = &zynqmp_pll_ops;
	pll->hw.clk.flags = nodes->flag | CLK_SET_RATE_PARENT;
	pll->hw.clk.parent_names = &pll->parent;
	pll->hw.clk.num_parents = 1;

	ret = bclk_register(&pll->hw.clk);
	if (ret) {
		kfree(pll);
		return ERR_PTR(ret);
	}

	return &pll->hw.clk;
}

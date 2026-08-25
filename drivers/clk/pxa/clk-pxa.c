// SPDX-License-Identifier: GPL-2.0-only
/*
 * Marvell PXA family clocks
 *
 * Copyright (C) 2014 Robert Jarzmik
 *
 * Common clock code for PXA clocks ("CKEN" type clocks + DT)
 *
 * Ported from the Linux kernel.
 */
#include <common.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <of.h>

#include <dt-bindings/clock/pxa-clock.h>
#include "clk-pxa.h"

static struct clk *pxa_clocks[CLK_MAX];
static struct clk_onecell_data onecell_data = {
	.clks = pxa_clocks,
	.clk_num = CLK_MAX,
};

/*
 * Unlike Linux, barebox's clk_hw embeds the struct clk itself and
 * clk_hw_register_composite() stores the ops in it, so the mux and the rate
 * part cannot share one clk_hw: the second assignment would overwrite the
 * ops of the first.
 */
struct pxa_clk {
	struct clk_hw mux_hw;
	struct clk_hw rate_hw;
	struct clk_fixed_factor lp;
	struct clk_fixed_factor hp;
	struct clk_gate gate;
	bool (*is_in_low_power)(void);
};

#define mux_to_pxa_clk(_hw) container_of(_hw, struct pxa_clk, mux_hw)
#define rate_to_pxa_clk(_hw) container_of(_hw, struct pxa_clk, rate_hw)

static unsigned long cken_recalc_rate(struct clk_hw *hw,
				      unsigned long parent_rate)
{
	struct pxa_clk *pclk = rate_to_pxa_clk(hw);
	struct clk_fixed_factor *fix;

	if (!pclk->is_in_low_power || pclk->is_in_low_power())
		fix = &pclk->lp;
	else
		fix = &pclk->hp;

	/*
	 * barebox's fixed factor only looks at mult and div, so unlike in
	 * Linux there is no need to point the embedded clk_hw at our parent
	 * first.
	 */
	return clk_fixed_factor_ops.recalc_rate(&fix->hw, parent_rate);
}

static const struct clk_ops cken_rate_ops = {
	.recalc_rate = cken_recalc_rate,
};

static int cken_get_parent(struct clk_hw *hw)
{
	struct pxa_clk *pclk = mux_to_pxa_clk(hw);

	if (!pclk->is_in_low_power)
		return 0;
	return pclk->is_in_low_power() ? 0 : 1;
}

static const struct clk_ops cken_mux_ops = {
	.get_parent = cken_get_parent,
	.set_parent = dummy_clk_set_parent,
};

void clkdev_pxa_register(int ckid, const char *con_id, const char *dev_id,
			 struct clk *clk)
{
	if (!IS_ERR(clk) && (ckid != CLK_NONE))
		pxa_clocks[ckid] = clk;
	if (!IS_ERR(clk))
		clk_register_clkdev(clk, con_id, dev_id);
}

int clk_pxa_cken_init(const struct desc_clk_cken *clks, int nb_clks,
		      void __iomem *clk_regs)
{
	struct pxa_clk *pxa_clk;
	struct clk_hw *hw;
	int i;

	for (i = 0; i < nb_clks; i++) {
		pxa_clk = xzalloc(sizeof(*pxa_clk));
		pxa_clk->is_in_low_power = clks[i].is_in_low_power;
		pxa_clk->lp = clks[i].lp;
		pxa_clk->hp = clks[i].hp;
		pxa_clk->gate = clks[i].gate;
		pxa_clk->gate.reg = clk_regs + clks[i].cken_reg;

		hw = clk_hw_register_composite(NULL, clks[i].name,
					       clks[i].parent_names, 2,
					       &pxa_clk->mux_hw, &cken_mux_ops,
					       &pxa_clk->rate_hw, &cken_rate_ops,
					       &pxa_clk->gate.hw, &clk_gate_ops,
					       clks[i].flags);
		clkdev_pxa_register(clks[i].ckid, clks[i].con_id,
				    clks[i].dev_id, clk_hw_to_clk(hw));
	}

	return 0;
}

int clk_pxa_dt_common_init(struct device_node *np)
{
	return of_clk_add_provider(np, of_clk_src_onecell_get, &onecell_data);
}

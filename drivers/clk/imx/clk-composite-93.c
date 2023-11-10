// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 NXP
 *
 * Peng Fan <peng.fan@nxp.com>
 */

#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/types.h>
#include <of_address.h>
#include <linux/iopoll.h>

#include "clk.h"

#define TIMEOUT_US	500U

#define CCM_DIV_SHIFT	0
#define CCM_DIV_WIDTH	8
#define CCM_MUX_SHIFT	8
#define CCM_MUX_MASK	3
#define CCM_OFF_SHIFT	24
#define CCM_BUSY_SHIFT	28

#define STAT_OFFSET	0x4
#define AUTHEN_OFFSET	0x30
#define TZ_NS_SHIFT	9
#define TZ_NS_MASK	BIT(9)

#define WHITE_LIST_SHIFT	16

static int imx93_clk_composite_wait_ready(struct clk_hw *hw, void __iomem *reg)
{
	int ret;
	u32 val;

	ret = readl_poll_timeout(reg + STAT_OFFSET, val, !(val & BIT(CCM_BUSY_SHIFT)),
				 TIMEOUT_US);
	if (ret)
		pr_err("Slice[%s] busy timeout\n", clk_hw_get_name(hw));

	return ret;
}

static void imx93_clk_composite_gate_endisable(struct clk_hw *hw, int enable)
{
	struct clk_gate *gate = to_clk_gate(hw);
	u32 reg;

	reg = readl(gate->reg);

	if (enable)
		reg &= ~BIT(gate->shift);
	else
		reg |= BIT(gate->shift);

	writel(reg, gate->reg);

	imx93_clk_composite_wait_ready(hw, gate->reg);
}

static int imx93_clk_composite_gate_enable(struct clk_hw *hw)
{
	imx93_clk_composite_gate_endisable(hw, 1);

	return 0;
}

static void imx93_clk_composite_gate_disable(struct clk_hw *hw)
{
	imx93_clk_composite_gate_endisable(hw, 0);
}

static const struct clk_ops imx93_clk_composite_gate_ops = {
	.enable = imx93_clk_composite_gate_enable,
	.disable = imx93_clk_composite_gate_disable,
	.is_enabled = clk_gate_is_enabled,
};

static unsigned long
imx93_clk_composite_divider_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	return clk_divider_ops.recalc_rate(hw, parent_rate);
}

static long
imx93_clk_composite_divider_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *prate)
{
	return clk_divider_ops.round_rate(hw, rate, prate);
}

static int imx93_clk_composite_divider_set_rate(struct clk_hw *hw, unsigned long rate,
						unsigned long parent_rate)
{
	struct clk_divider *divider = to_clk_divider(hw);
	int value;
	u32 val;
	int ret;

	value = divider_get_val(rate, parent_rate, divider->table, divider->width, divider->flags);
	if (value < 0)
		return value;

	val = readl(divider->reg);
	val &= ~(clk_div_mask(divider->width) << divider->shift);
	val |= (u32)value << divider->shift;
	writel(val, divider->reg);

	ret = imx93_clk_composite_wait_ready(hw, divider->reg);

	return ret;
}

static const struct clk_ops imx93_clk_composite_divider_ops = {
	.recalc_rate = imx93_clk_composite_divider_recalc_rate,
	.round_rate = imx93_clk_composite_divider_round_rate,
	.set_rate = imx93_clk_composite_divider_set_rate,
};

static int imx93_clk_composite_mux_get_parent(struct clk_hw *hw)
{
	return clk_mux_ops.get_parent(hw);
}

static int imx93_clk_composite_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_mux *mux = to_clk_mux(hw);
	u32 val = clk_mux_index_to_val(mux->table, mux->flags, index);
	u32 reg;
	int ret;

	reg = readl(mux->reg);
	reg &= ~(((1 << mux->width) - 1) << mux->shift);
	val = val << mux->shift;
	reg |= val;
	writel(reg, mux->reg);

	ret = imx93_clk_composite_wait_ready(hw, mux->reg);

	return ret;
}

static const struct clk_ops imx93_clk_composite_mux_ops = {
	.get_parent = imx93_clk_composite_mux_get_parent,
	.set_parent = imx93_clk_composite_mux_set_parent,
};

struct clk *imx93_clk_composite_flags(const char *name, const char * const *parent_names,
				      int num_parents, void __iomem *reg, u32 domain_id,
				      unsigned long flags)
{
	struct clk_hw *hw = ERR_PTR(-ENOMEM), *mux_hw;
	struct clk_hw *div_hw, *gate_hw;
	struct clk_divider *div = NULL;
	struct clk_gate *gate = NULL;
	struct clk_mux *mux = NULL;
	bool clk_ro = false;
	u32 authen;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		goto fail;

	mux_hw = &mux->hw;
	mux->reg = reg;
	mux->shift = CCM_MUX_SHIFT;
	mux->width = 2;

	div = kzalloc(sizeof(*div), GFP_KERNEL);
	if (!div)
		goto fail;

	div_hw = &div->hw;
	div->reg = reg;
	div->shift = CCM_DIV_SHIFT;
	div->width = CCM_DIV_WIDTH;
//	div->flags = CLK_DIVIDER_ROUND_CLOSEST;

	authen = readl(reg + AUTHEN_OFFSET);
	if (!(authen & TZ_NS_MASK) || !(authen & BIT(WHITE_LIST_SHIFT + domain_id)))
		clk_ro = true;

	if (clk_ro) {
		hw = clk_hw_register_composite(NULL, name, parent_names, num_parents,
					       mux_hw, &clk_mux_ro_ops, div_hw,
					       &clk_divider_ro_ops, NULL, NULL, flags);
	} else {
		gate = kzalloc(sizeof(*gate), GFP_KERNEL);
		if (!gate)
			goto fail;

		gate_hw = &gate->hw;
		gate->reg = reg;
		gate->shift = CCM_OFF_SHIFT;
		gate->flags = CLK_GATE_SET_TO_DISABLE;

		hw = clk_hw_register_composite(NULL, name, parent_names, num_parents,
					       mux_hw, &imx93_clk_composite_mux_ops, div_hw,
					       &imx93_clk_composite_divider_ops, gate_hw,
					       &imx93_clk_composite_gate_ops,
					       flags | CLK_SET_RATE_NO_REPARENT);
	}

	if (IS_ERR(hw))
		goto fail;

	return &hw->clk;

fail:
	kfree(gate);
	kfree(div);
	kfree(mux);
	return ERR_CAST(hw);
}
EXPORT_SYMBOL_GPL(imx93_clk_composite_flags);

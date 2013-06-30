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
#include <io.h>
#include <malloc.h>
#include <asm-generic/div64.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clk.h"

#define pll_out_override(p) (BIT((p->shift - 6)))
#define div_mask(d) ((1 << (d->width)) - 1)
#define get_mul(d) (1 << d->frac_width)
#define get_max_div(d) div_mask(d)

#define PERIPH_CLK_UART_DIV_ENB BIT(24)

#define to_clk_frac_div(_hw) container_of(_hw, struct tegra_clk_frac_div, hw)

static int get_div(struct tegra_clk_frac_div *divider, unsigned long rate,
		   unsigned long parent_rate)
{
	u64 divider_ux1 = parent_rate;
	u8 flags = divider->flags;
	int mul;

	if (!rate)
		return 0;

	mul = get_mul(divider);

	if (!(flags & TEGRA_DIVIDER_INT))
		divider_ux1 *= mul;

	if (flags & TEGRA_DIVIDER_ROUND_UP)
		divider_ux1 += rate - 1;

	do_div(divider_ux1, rate);

	if (flags & TEGRA_DIVIDER_INT)
		divider_ux1 *= mul;

	divider_ux1 -= mul;

	if (divider_ux1 < 0)
		return 0;

	if (divider_ux1 > get_max_div(divider))
		return -EINVAL;

	return divider_ux1;
}

static unsigned long clk_frac_div_recalc_rate(struct clk *hw,
					     unsigned long parent_rate)
{
	struct tegra_clk_frac_div *divider = to_clk_frac_div(hw);
	u32 reg;
	int div, mul;
	u64 rate = parent_rate;

	reg = readl(divider->reg) >> divider->shift;
	div = reg & div_mask(divider);

	mul = get_mul(divider);
	div += mul;

	rate *= mul;
	rate += div - 1;
	do_div(rate, div);

	return rate;
}

static long clk_frac_div_round_rate(struct clk *hw, unsigned long rate,
				   unsigned long *prate)
{
	struct tegra_clk_frac_div *divider = to_clk_frac_div(hw);
	int div, mul;
	unsigned long output_rate = *prate;

	if (!rate)
		return output_rate;

	div = get_div(divider, rate, output_rate);
	if (div < 0)
		return *prate;

	mul = get_mul(divider);

	return DIV_ROUND_UP(output_rate * mul, div + mul);
}

static int clk_frac_div_set_rate(struct clk *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct tegra_clk_frac_div *divider = to_clk_frac_div(hw);
	int div;
	u32 val;

	div = get_div(divider, rate, parent_rate);
	if (div < 0)
		return div;

	val = readl(divider->reg);
	val &= ~(div_mask(divider) << divider->shift);
	val |= div << divider->shift;

	if (divider->flags & TEGRA_DIVIDER_UART) {
		if (div)
			val |= PERIPH_CLK_UART_DIV_ENB;
		else
			val &= ~PERIPH_CLK_UART_DIV_ENB;
	}

	if (divider->flags & TEGRA_DIVIDER_FIXED)
		val |= pll_out_override(divider);

	writel(val, divider->reg);

	return 0;
}

const struct clk_ops tegra_clk_frac_div_ops = {
	.recalc_rate = clk_frac_div_recalc_rate,
	.set_rate = clk_frac_div_set_rate,
	.round_rate = clk_frac_div_round_rate,
};

struct clk *tegra_clk_divider_alloc(const char *name, const char *parent_name,
		void __iomem *reg, unsigned long flags, u8 clk_divider_flags,
		u8 shift, u8 width, u8 frac_width)
{
	struct tegra_clk_frac_div *divider;

	divider = kzalloc(sizeof(*divider), GFP_KERNEL);
	if (!divider) {
		pr_err("%s: could not allocate fractional divider clk\n",
		       __func__);
		return NULL;
	}

	divider->parent = parent_name;
	divider->hw.name = name;
	divider->hw.ops = &tegra_clk_frac_div_ops;
	divider->hw.flags = flags;
	divider->hw.parent_names = divider->parent ? &divider->parent : NULL;
	divider->hw.num_parents = divider->parent ? 1 : 0;

	divider->reg = reg;
	divider->shift = shift;
	divider->width = width;
	divider->frac_width = frac_width;
	divider->flags = clk_divider_flags;

	return &divider->hw;
}

void tegra_clk_divider_free(struct clk *clk_div)
{
	struct tegra_clk_frac_div *divider = to_clk_frac_div(clk_div);

	kfree(divider);
}

struct clk *tegra_clk_register_divider(const char *name,
		const char *parent_name, void __iomem *reg, unsigned long flags,
		u8 clk_divider_flags, u8 shift, u8 width, u8 frac_width)
{
	struct tegra_clk_frac_div *divider;
	int ret;

	divider = to_clk_frac_div(tegra_clk_divider_alloc(name, parent_name,
				  reg, flags, clk_divider_flags, shift, width,
				  frac_width));

	ret = clk_register(&divider->hw);
	if (ret) {
		kfree(divider);
		return ERR_PTR(ret);
	}

	return &divider->hw;
}

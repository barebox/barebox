// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 MundoReader S.L.
 * Author: Heiko Stuebner <heiko@sntech.de>
 *
 * Copyright (c) 2016 Rockchip Electronics Co. Ltd.
 * Author: Xing Zheng <zhengxing@rock-chips.com>
 *
 * based on
 *
 * samsung/clk.c
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 * Copyright (c) 2013 Linaro Ltd.
 * Author: Thomas Abraham <thomas.ab@samsung.com>
 */

#include <common.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <mfd/syscon.h>
#include <linux/rational.h>
#include <restart.h>
#include "clk.h"

#include "../clk-fractional-divider.h"

/*
 * Register a clock branch.
 * Most clock branches have a form like
 *
 * src1 --|--\
 *        |M |--[GATE]-[DIV]-
 * src2 --|--/
 *
 * sometimes without one of those components.
 */
static struct clk *rockchip_clk_register_branch(const char *name,
		const char *const *parent_names, u8 num_parents,
		void __iomem *base,
		int muxdiv_offset, u8 mux_shift, u8 mux_width, u8 mux_flags,
		int div_offset, u8 div_shift, u8 div_width, u8 div_flags,
		struct clk_div_table *div_table, int gate_offset,
		u8 gate_shift, u8 gate_flags, unsigned long flags,
		spinlock_t *lock)
{
	struct clk *clk;
	struct clk_mux *mux = NULL;
	struct clk_gate *gate = NULL;
	struct clk_divider *div = NULL;
	int ret;

	if (num_parents > 1) {
		mux = kzalloc(sizeof(*mux), GFP_KERNEL);
		if (!mux)
			return ERR_PTR(-ENOMEM);

		mux->reg = base + muxdiv_offset;
		mux->shift = mux_shift;
		mux->mask = BIT(mux_width) - 1;
		mux->flags = mux_flags;
		mux->lock = lock;
		mux->hw.clk.name = basprintf("%s.mux", name);
		mux->hw.clk.ops = (mux_flags & CLK_MUX_READ_ONLY) ? &clk_mux_ro_ops
							: &clk_mux_ops;
	}

	if (gate_offset >= 0) {
		gate = kzalloc(sizeof(*gate), GFP_KERNEL);
		if (!gate) {
			ret = -ENOMEM;
			goto err_gate;
		}

		gate->flags = gate_flags;
		gate->reg = base + gate_offset;
		gate->shift = gate_shift;
		gate->lock = lock;
		gate->hw.clk.name = basprintf("%s.gate", name);
		gate->hw.clk.ops = &clk_gate_ops;
	}

	if (div_width > 0) {
		div = kzalloc(sizeof(*div), GFP_KERNEL);
		if (!div) {
			ret = -ENOMEM;
			goto err_div;
		}

		div->flags = div_flags;
		if (div_offset)
			div->reg = base + div_offset;
		else
			div->reg = base + muxdiv_offset;
		div->shift = div_shift;
		div->width = div_width;
		div->lock = lock;
		div->table = div_table;
		div->hw.clk.name = basprintf("%s.div", name);
		div->hw.clk.ops = (div_flags & CLK_DIVIDER_READ_ONLY)
						? &clk_divider_ro_ops
						: &clk_divider_ops;
	}

	clk = clk_register_composite(name, parent_names, num_parents,
				       mux ? &mux->hw.clk : NULL,
				       div ? &div->hw.clk : NULL,
				       gate ? &gate->hw.clk : NULL,
				       flags);
	if (IS_ERR(clk)) {
		kfree(div);
		kfree(gate);
		return ERR_CAST(clk);
	}

	return clk;
err_div:
	kfree(gate);
err_gate:
	kfree(mux);
	return ERR_PTR(ret);
}

struct rockchip_clk_frac {
	struct clk_fractional_divider		div;
	struct clk_gate				gate;

	struct clk_mux				mux;
	const struct clk_ops			*mux_ops;
	int					mux_frac_idx;

	bool					rate_change_remuxed;
	int					rate_change_idx;
};

/*
 * fractional divider must set that denominator is 20 times larger than
 * numerator to generate precise clock frequency.
 */
static void rockchip_fractional_approximation(struct clk_hw *hw,
		unsigned long rate, unsigned long *parent_rate,
		unsigned long *m, unsigned long *n)
{
	struct clk_fractional_divider *fd = to_clk_fd(hw);
	unsigned long p_rate, p_parent_rate;
	struct clk_hw *p_parent;

	p_rate = clk_hw_get_rate(clk_hw_get_parent(hw));
	if ((rate * 20 > p_rate) && (p_rate % rate != 0)) {
		p_parent = clk_hw_get_parent(clk_hw_get_parent(hw));
		p_parent_rate = clk_hw_get_rate(p_parent);
		*parent_rate = p_parent_rate;
	}

	fd->flags |= CLK_FRAC_DIVIDER_POWER_OF_TWO_PS;

	clk_fractional_divider_general_approximation(hw, rate, parent_rate, m, n);
}

static struct clk *rockchip_clk_register_frac_branch(
		struct rockchip_clk_provider *ctx, const char *name,
		const char *const *parent_names, u8 num_parents,
		void __iomem *base, int muxdiv_offset, u8 div_flags,
		int gate_offset, u8 gate_shift, u8 gate_flags,
		unsigned long flags, struct rockchip_clk_branch *child,
		spinlock_t *lock)
{
	struct clk *clk;
	struct rockchip_clk_frac *frac;
	struct clk_gate *gate = NULL;
	struct clk_fractional_divider *div = NULL;

	if (muxdiv_offset < 0)
		return ERR_PTR(-EINVAL);

	if (child && child->branch_type != branch_mux) {
		pr_err("%s: fractional child clock for %s can only be a mux\n",
		       __func__, name);
		return ERR_PTR(-EINVAL);
	}

	frac = kzalloc(sizeof(*frac), GFP_KERNEL);
	if (!frac)
		return ERR_PTR(-ENOMEM);

	if (gate_offset >= 0) {
		gate = &frac->gate;
		gate->flags = gate_flags;
		gate->reg = base + gate_offset;
		gate->shift = gate_shift;
		gate->lock = lock;
		gate->hw.clk.ops = &clk_gate_ops;
	}

	div = &frac->div;
	div->flags = div_flags;
	div->reg = base + muxdiv_offset;
	div->mshift = 16;
	div->mwidth = 16;
	div->nshift = 0;
	div->nwidth = 16;
	div->lock = lock;
	div->approximation = rockchip_fractional_approximation;
	div->hw.clk.ops = &clk_fractional_divider_ops;

	clk = clk_register_composite(name, parent_names, num_parents,
				       NULL,
				       &div->hw.clk,
				       gate ? &gate->hw.clk : NULL,
				       flags | CLK_SET_RATE_UNGATE);
	if (IS_ERR(clk)) {
		kfree(frac);
		return ERR_CAST(clk);
	}

	if (child) {
		struct clk_mux *frac_mux = &frac->mux;
		struct clk_init_data init;
		struct clk *mux_clk;

		frac->mux_frac_idx = match_string(child->parent_names,
						  child->num_parents, name);
		frac->mux_ops = &clk_mux_ops;

		frac_mux->reg = base + child->muxdiv_offset;
		frac_mux->shift = child->mux_shift;
		frac_mux->mask = BIT(child->mux_width) - 1;
		frac_mux->flags = child->mux_flags;
		frac_mux->lock = lock;
		frac_mux->hw.init = &init;

		init.name = child->name;
		init.flags = child->flags | CLK_SET_RATE_PARENT;
		init.ops = frac->mux_ops;
		init.parent_names = child->parent_names;
		init.num_parents = child->num_parents;

		mux_clk = clk_register(NULL, &frac_mux->hw);
		if (IS_ERR(mux_clk)) {
			kfree(frac);
			return mux_clk;
		}

		rockchip_clk_set_lookup(ctx, mux_clk, child->id);

		/* notifier on the fraction divider to catch rate changes */
		if (frac->mux_frac_idx >= 0) {
			pr_debug("%s: found fractional parent in mux at pos %d\n",
				 __func__, frac->mux_frac_idx);
		} else {
			pr_warn("%s: could not find %s as parent of %s, rate changes may not work\n",
				__func__, name, child->name);
		}
	}

	return clk;
}

static struct clk *rockchip_clk_register_factor_branch(const char *name,
		const char *const *parent_names, u8 num_parents,
		void __iomem *base, unsigned int mult, unsigned int div,
		int gate_offset, u8 gate_shift, u8 gate_flags,
		unsigned long flags, spinlock_t *lock)
{
	struct clk *clk;
	struct clk_gate *gate = NULL;
	struct clk_fixed_factor *fix = NULL;

	/* without gate, register a simple factor clock */
	if (gate_offset == 0) {
		return clk_register_fixed_factor(NULL, name,
				parent_names[0], flags, mult,
				div);
	}

	gate = kzalloc(sizeof(*gate), GFP_KERNEL);
	if (!gate)
		return ERR_PTR(-ENOMEM);

	gate->flags = gate_flags;
	gate->reg = base + gate_offset;
	gate->shift = gate_shift;
	gate->lock = lock;
	gate->hw.clk.ops = &clk_gate_ops;

	fix = kzalloc(sizeof(*fix), GFP_KERNEL);
	if (!fix) {
		kfree(gate);
		return ERR_PTR(-ENOMEM);
	}

	fix->mult = mult;
	fix->div = div;
	fix->hw.clk.ops = &clk_fixed_factor_ops;

	clk = clk_register_composite(name, parent_names, num_parents,
				       NULL,
				       &fix->hw.clk,
				       &gate->hw.clk, flags);
	if (IS_ERR(clk)) {
		kfree(fix);
		kfree(gate);
		return ERR_CAST(clk);
	}

	return clk;
}

struct rockchip_clk_provider *rockchip_clk_init(struct device_node *np,
						void __iomem *base,
						unsigned long nr_clks)
{
	struct rockchip_clk_provider *ctx;
	struct clk **clk_table;
	int i;

	ctx = kzalloc(sizeof(struct rockchip_clk_provider), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	clk_table = kcalloc(nr_clks, sizeof(struct clk *), GFP_KERNEL);
	if (!clk_table)
		goto err_free;

	for (i = 0; i < nr_clks; ++i)
		clk_table[i] = ERR_PTR(-ENOENT);

	ctx->reg_base = base;
	ctx->clk_data.clks = clk_table;
	ctx->clk_data.clk_num = nr_clks;
	ctx->cru_node = np;
	spin_lock_init(&ctx->lock);

	ctx->grf = syscon_regmap_lookup_by_phandle(ctx->cru_node,
						   "rockchip,grf");
	ctx->grfmap[grf_type_sys] = ctx->grf;

	return ctx;

err_free:
	kfree(ctx);
	return ERR_PTR(-ENOMEM);
}
EXPORT_SYMBOL_GPL(rockchip_clk_init);

void rockchip_clk_of_add_provider(struct device_node *np,
				  struct rockchip_clk_provider *ctx)
{
	if (of_clk_add_provider(np, of_clk_src_onecell_get,
				&ctx->clk_data))
		pr_err("%s: could not register clk provider\n", __func__);
}
EXPORT_SYMBOL_GPL(rockchip_clk_of_add_provider);

void rockchip_clk_register_plls(struct rockchip_clk_provider *ctx,
				struct rockchip_pll_clock *list,
				unsigned int nr_pll, int grf_lock_offset)
{
	struct clk *clk;
	int idx;

	for (idx = 0; idx < nr_pll; idx++, list++) {
		clk = rockchip_clk_register_pll(ctx, list->type, list->name,
				list->parent_names, list->num_parents,
				list->con_offset, grf_lock_offset,
				list->lock_shift, list->mode_offset,
				list->mode_shift, list->rate_table,
				list->flags, list->pll_flags);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				list->name);
			continue;
		}

		rockchip_clk_set_lookup(ctx, clk, list->id);
	}
}
EXPORT_SYMBOL_GPL(rockchip_clk_register_plls);

unsigned long rockchip_clk_find_max_clk_id(struct rockchip_clk_branch *list,
					   unsigned int nr_clk)
{
	unsigned long max = 0;
	unsigned int idx;

	for (idx = 0; idx < nr_clk; idx++, list++) {
		if (list->id > max)
			max = list->id;
		if (list->child && list->child->id > max)
			max = list->child->id;
	}

	return max;
}
EXPORT_SYMBOL_GPL(rockchip_clk_find_max_clk_id);

static struct device *rockchip_clk_register_gate_link(
		struct device *parent_dev,
		struct rockchip_clk_provider *ctx,
		struct rockchip_clk_branch *clkbr)
{
	struct rockchip_gate_link_platdata gate_link_pdata = {
		.ctx = ctx,
		.clkbr = clkbr,
	};
	struct device *dev;
	int ret;

	dev = device_alloc("rockchip-gate-link-clk", clkbr->id);
	dev->parent = parent_dev;
	dev->platform_data = &gate_link_pdata;

	ret = platform_device_register(dev);
	if (ret)
		return ERR_PTR(ret);

	return dev;
}

void rockchip_clk_register_branches(struct rockchip_clk_provider *ctx,
				    struct rockchip_clk_branch *list,
				    unsigned int nr_clk)
{
	struct clk *clk = NULL;
	unsigned int idx;
	unsigned long flags;

	for (idx = 0; idx < nr_clk; idx++, list++) {
		flags = list->flags;

		/* catch simple muxes */
		switch (list->branch_type) {
		case branch_mux:
			clk = clk_register_mux(NULL, list->name,
				list->parent_names, list->num_parents,
				flags, ctx->reg_base + list->muxdiv_offset,
				list->mux_shift, list->mux_width,
				list->mux_flags, &ctx->lock);
			break;
		case branch_muxgrf:
			clk = rockchip_clk_register_muxgrf(list->name,
				list->parent_names, list->num_parents,
				flags, ctx->grfmap[list->grf_type],
				list->muxdiv_offset,
				list->mux_shift, list->mux_width,
				list->mux_flags);
			break;
		case branch_divider:
			if (list->div_table)
				clk = clk_register_divider_table(NULL,
					list->name, list->parent_names[0],
					flags,
					ctx->reg_base + list->muxdiv_offset,
					list->div_shift, list->div_width,
					list->div_flags, list->div_table,
					&ctx->lock);
			else
				clk = clk_register_divider(NULL, list->name,
					list->parent_names[0], flags,
					ctx->reg_base + list->muxdiv_offset,
					list->div_shift, list->div_width,
					list->div_flags, &ctx->lock);
			break;
		case branch_fraction_divider:
			clk = rockchip_clk_register_frac_branch(ctx, list->name,
				list->parent_names, list->num_parents,
				ctx->reg_base, list->muxdiv_offset,
				list->div_flags,
				list->gate_offset, list->gate_shift,
				list->gate_flags, flags, list->child,
				&ctx->lock);
			break;
		case branch_half_divider:
			break;
		case branch_gate:
			flags |= CLK_SET_RATE_PARENT;

			clk = clk_register_gate(NULL, list->name,
				list->parent_names[0], flags,
				ctx->reg_base + list->gate_offset,
				list->gate_shift, list->gate_flags, &ctx->lock);
			break;
		case branch_grf_gate:
			flags |= CLK_SET_RATE_PARENT;
			/* FIXME:
			clk = rockchip_clk_register_gate_grf(list->name,
				list->parent_names[0], flags, grf,
				list->gate_offset, list->gate_shift,
				list->gate_flags);
			*/
			break;
		case branch_composite:
			clk = rockchip_clk_register_branch(list->name,
				list->parent_names, list->num_parents,
				ctx->reg_base, list->muxdiv_offset,
				list->mux_shift,
				list->mux_width, list->mux_flags,
				list->div_offset, list->div_shift, list->div_width,
				list->div_flags, list->div_table,
				list->gate_offset, list->gate_shift,
				list->gate_flags, flags, &ctx->lock);
			break;
		case branch_mmc:
			clk = rockchip_clk_register_mmc(
				list->name,
				list->parent_names, list->num_parents,
				ctx->reg_base + list->muxdiv_offset,
				list->div_shift
			);
			break;
		case branch_inverter:
			clk = rockchip_clk_register_inverter(
				list->name, list->parent_names,
				list->num_parents,
				ctx->reg_base + list->muxdiv_offset,
				list->div_shift, list->div_flags, &ctx->lock);
			break;
		case branch_factor:
			clk = rockchip_clk_register_factor_branch(
				list->name, list->parent_names,
				list->num_parents, ctx->reg_base,
				list->div_shift, list->div_width,
				list->gate_offset, list->gate_shift,
				list->gate_flags, flags, &ctx->lock);
			break;
		case branch_ddrclk:
			break;
		case branch_linked_gate:
			/* must be registered late, fall-through for error message */
			break;
		}

		/* none of the cases above matched */
		if (!clk) {
			pr_err("%s: unknown clock type %d\n",
			       __func__, list->branch_type);
			continue;
		}

		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s: %ld\n",
			       __func__, list->name, PTR_ERR(clk));
			continue;
		}

		rockchip_clk_set_lookup(ctx, clk, list->id);
	}
}
EXPORT_SYMBOL_GPL(rockchip_clk_register_branches);

void rockchip_clk_register_late_branches(struct device *dev,
					 struct rockchip_clk_provider *ctx,
					 struct rockchip_clk_branch *list,
					 unsigned int nr_clk)
{
	unsigned int idx;

	for (idx = 0; idx < nr_clk; idx++, list++) {
		struct device *pdev = NULL;

		switch (list->branch_type) {
		case branch_linked_gate:
			pdev = rockchip_clk_register_gate_link(dev, ctx, list);
			break;
		default:
			dev_err(dev, "unknown clock type %d\n", list->branch_type);
			break;
		}

		if (!pdev)
			dev_err(dev, "failed to register device for clock %s\n", list->name);
	}
}
EXPORT_SYMBOL_GPL(rockchip_clk_register_late_branches);

void rockchip_clk_register_armclk(struct rockchip_clk_provider *ctx,
				  unsigned int lookup_id,
				  const char *name, const char *const *parent_names,
				  u8 num_parents,
				  const struct rockchip_cpuclk_reg_data *reg_data,
				  const struct rockchip_cpuclk_rate_table *rates,
				  int nrates)
{
	struct clk *clk;

	clk = rockchip_clk_register_cpuclk(name, parent_names, num_parents,
					   reg_data, rates, nrates,
					   ctx->reg_base, &ctx->lock);
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register clock %s: %ld\n",
		       __func__, name, PTR_ERR(clk));
		return;
	}

	rockchip_clk_set_lookup(ctx, clk, lookup_id);
}
EXPORT_SYMBOL_GPL(rockchip_clk_register_armclk);

void rockchip_clk_protect_critical(const char *const clocks[],
				   int nclocks)
{
	int i;

	/* Protect the clocks that needs to stay on */
	for (i = 0; i < nclocks; i++) {
		struct clk *clk = __clk_lookup(clocks[i]);

		clk_enable(clk);
	}
}
EXPORT_SYMBOL_GPL(rockchip_clk_protect_critical);

static void rockchip_restart(struct restart_handler *this,
			     unsigned long flags)
{
	struct rockchip_clk_provider *ctx =
		container_of(this, struct rockchip_clk_provider, restart_handler);

	writel(0xfdb9, ctx->reg_base + ctx->reg_restart);
	mdelay(1000);
}

void rockchip_register_restart_notifier(struct rockchip_clk_provider *ctx,
					unsigned int reg)
{
	int ret;

	ctx->restart_handler.name = "rockchip-pmu",
	ctx->restart_handler.restart = rockchip_restart,
	ctx->restart_handler.priority = RESTART_DEFAULT_PRIORITY,

	ctx->reg_restart = reg;

	ret = restart_handler_register(&ctx->restart_handler);
	if (ret)
		pr_err("%s: cannot register restart handler, %d\n",
		       __func__, ret);
}
EXPORT_SYMBOL_GPL(rockchip_register_restart_notifier);

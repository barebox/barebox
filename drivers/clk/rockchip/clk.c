/*
 * Copyright (c) 2014 MundoReader S.L.
 * Author: Heiko Stuebner <heiko@sntech.de>
 *
 * based on
 *
 * samsung/clk.c
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 * Copyright (c) 2013 Linaro Ltd.
 * Author: Thomas Abraham <thomas.ab@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <malloc.h>
#include <linux/clk.h>
#include "clk.h"
#include <init.h>

/**
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
		const char **parent_names, u8 num_parents, void __iomem *base,
		int muxdiv_offset, u8 mux_shift, u8 mux_width, u8 mux_flags,
		u8 div_shift, u8 div_width, u8 div_flags,
		struct clk_div_table *div_table, int gate_offset,
		u8 gate_shift, u8 gate_flags, unsigned long flags
		)
{
	struct clk *clk;
	struct clk *mux = NULL;
	struct clk *gate = NULL;
	struct clk *div = NULL;

	if (num_parents > 1) {
		mux = clk_mux_alloc(name, base + muxdiv_offset, mux_shift,
		    mux_width, parent_names, num_parents, mux_flags);
		if (!mux)
			return ERR_PTR(-ENOMEM);
	}

	if (gate_offset >= 0) {
		gate = clk_gate_alloc(name, *parent_names, base + gate_offset,
		    gate_shift, flags, gate_flags);
		if (!gate)
			return ERR_PTR(-ENOMEM);
	}

	if (div_width > 0) {
		div = clk_divider_alloc(name, *parent_names,
		    base + muxdiv_offset, div_shift, div_width, div_flags);
		if (!div)
			return ERR_PTR(-ENOMEM);
	}

	clk = clk_register_composite(name, parent_names, num_parents,
				     mux,
				     div,
				     gate,
				     flags);

	return clk;
}

static struct clk *rockchip_clk_register_frac_branch(const char *name,
		const char **parent_names, u8 num_parents, void __iomem *base,
		int muxdiv_offset, u8 div_flags,
		int gate_offset, u8 gate_shift, u8 gate_flags,
		unsigned long flags)
{
	struct clk *clk;
	struct clk *gate = NULL;
	struct clk *div = NULL;

	if (gate_offset >= 0) {
		gate = clk_gate_alloc(name, *parent_names, base + gate_offset,
		    gate_shift, flags, gate_flags);
		if (!gate)
			return ERR_PTR(-ENOMEM);
	}

	if (muxdiv_offset < 0)
		return ERR_PTR(-EINVAL);

	div = clk_fractional_divider_alloc(name, *parent_names, flags,
		     base + muxdiv_offset, 16, 16, 0, 16, div_flags);
	if (!div)
		return ERR_PTR(-ENOMEM);

	clk = clk_register_composite(name, parent_names, num_parents,
				     NULL,
				     div,
				     gate,
				     flags);

	return clk;
}

static struct clk **clk_table;
static void __iomem *reg_base;
static struct clk_onecell_data clk_data;
static struct device_node *cru_node;

void __init rockchip_clk_init(struct device_node *np, void __iomem *base,
			      unsigned long nr_clks)
{
	reg_base = base;
	cru_node = np;

	clk_table = calloc(nr_clks, sizeof(struct clk *));
	if (!clk_table)
		pr_err("%s: could not allocate clock lookup table\n", __func__);

	clk_data.clks = clk_table;
	clk_data.clk_num = nr_clks;
	of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
}

void rockchip_clk_add_lookup(struct clk *clk, unsigned int id)
{
	if (clk_table && id)
		clk_table[id] = clk;
}

void __init rockchip_clk_register_plls(struct rockchip_pll_clock *list,
				unsigned int nr_pll, int grf_lock_offset)
{
	struct clk *clk;
	int idx;

	for (idx = 0; idx < nr_pll; idx++, list++) {
		clk = rockchip_clk_register_pll(list->type, list->name,
				list->parent_names, list->num_parents,
				reg_base, list->con_offset, grf_lock_offset,
				list->lock_shift, list->mode_offset,
				list->mode_shift, list->rate_table,
				list->pll_flags);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				list->name);
			continue;
		}

		rockchip_clk_add_lookup(clk, list->id);
	}
}

void __init rockchip_clk_register_branches(
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
			/*
			 * mux_flags and flags are ored, this is safe,
			 * since there is no value clash, but isn't that elegant
			 */
			clk = clk_mux(list->name,
			    reg_base + list->muxdiv_offset, list->mux_shift,
			    list->mux_width, list->parent_names,
			    list->num_parents, list->mux_flags | flags);
			break;
		case branch_divider:
			if (list->div_table)
				clk = clk_divider_table(list->name,
				    list->parent_names[0],
				    reg_base + list->muxdiv_offset,
				    list->div_shift, list->div_width,
				    list->div_table, list->div_flags);
			else
				clk = clk_divider(list->name,
				    list->parent_names[0],
				    reg_base + list->muxdiv_offset,
				    list->div_shift, list->div_width,
				    list->div_flags);
			break;
		case branch_fraction_divider:
			clk = rockchip_clk_register_frac_branch(list->name,
				list->parent_names, list->num_parents,
				reg_base, list->muxdiv_offset, list->div_flags,
				list->gate_offset, list->gate_shift,
				list->gate_flags, flags);
			break;
		case branch_gate:
			flags |= CLK_SET_RATE_PARENT;

			clk = clk_gate(list->name, list->parent_names[0],
			    reg_base + list->gate_offset, list->gate_shift,
			    flags, list->gate_flags);
			break;
		case branch_composite:
			clk = rockchip_clk_register_branch(list->name,
				list->parent_names, list->num_parents,
				reg_base, list->muxdiv_offset, list->mux_shift,
				list->mux_width, list->mux_flags,
				list->div_shift, list->div_width,
				list->div_flags, list->div_table,
				list->gate_offset, list->gate_shift,
				list->gate_flags, flags);
			break;
		case branch_mmc:
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

		rockchip_clk_add_lookup(clk, list->id);
	}
}

void __init rockchip_clk_register_armclk(unsigned int lookup_id,
			const char *name, const char **parent_names,
			u8 num_parents,
			const struct rockchip_cpuclk_reg_data *reg_data,
			const struct rockchip_cpuclk_rate_table *rates,
			int nrates)
{
	struct clk *clk;

	clk = rockchip_clk_register_cpuclk(name, parent_names, num_parents,
					   reg_data, rates, nrates, reg_base
					   );
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register clock %s: %ld\n",
		       __func__, name, PTR_ERR(clk));
		return;
	}

	rockchip_clk_add_lookup(clk, lookup_id);
}

void __init rockchip_clk_protect_critical(const char *clocks[], int nclocks)
{
	int i;

	/* Protect the clocks that needs to stay on */
	for (i = 0; i < nclocks; i++) {
		struct clk *clk = __clk_lookup(clocks[i]);

		if (clk)
			clk_enable(clk);
	}
}

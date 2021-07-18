/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2021 Ahmad Fatoum, Pengutronix
 */

#ifndef STARFIVE_CLK_H_
#define STARFIVE_CLK_H_

#include <linux/clk.h>

#define STARFIVE_CLK_ENABLE_SHIFT	31
#define STARFIVE_CLK_INVERT_SHIFT	30
#define STARFIVE_CLK_MUX_SHIFT		24

static inline struct clk *starfive_clk_underspecifid(const char *name, const char *parent)
{
	/*
	 * TODO With documentation available, all users of this functions can be
	 * migrated to one of the above or to a clk_fixed_factor with
	 * appropriate factor
	 */
	return clk_fixed_factor(name, parent, 1, 1, 0);
}

static inline struct clk *starfive_clk_divider(const char *name, const char *parent,
		void __iomem *reg, u8 width)
{
	return starfive_clk_underspecifid(name, parent);
}

static inline struct clk *starfive_clk_gate(const char *name, const char *parent,
		void __iomem *reg)
{
	return clk_gate(name, parent, reg, STARFIVE_CLK_ENABLE_SHIFT, CLK_SET_RATE_PARENT, 0);
}

static inline struct clk *starfive_clk_divider_table(const char *name,
		const char *parent, void __iomem *reg, u8 width,
		const struct clk_div_table *table)
{
	return clk_divider_table(name, parent, CLK_SET_RATE_PARENT, reg, 0,
				 width, table, 0);
}

static inline struct clk *starfive_clk_gated_divider(const char *name,
		const char *parent, void __iomem *reg, u8 width)
{
	/* TODO divider part */
	return clk_gate(name, parent, reg, STARFIVE_CLK_ENABLE_SHIFT, CLK_SET_RATE_PARENT, 0);
}

static inline struct clk *starfive_clk_gate_dis(const char *name, const char *parent,
		void __iomem *reg)
{
	return clk_gate_inverted(name, parent, reg, STARFIVE_CLK_INVERT_SHIFT, CLK_SET_RATE_PARENT);
}

static inline struct clk *starfive_clk_mux(const char *name, void __iomem *reg,
		u8 width, const char * const *parents, u8 num_parents)
{
	return clk_mux(name, 0, reg, STARFIVE_CLK_MUX_SHIFT, width, parents, num_parents, 0);
}

#endif

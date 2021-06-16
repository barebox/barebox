// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clk.h"

/**
 * struct clk_gate_exclusive - i.MX specific gate clock which is mutually
 * exclusive with other gate clocks
 *
 * @gate: the parent class
 * @exclusive_mask: mask of gate bits which are mutually exclusive to this
 *	gate clock
 *
 * The imx exclusive gate clock is a subclass of basic clk_gate
 * with an addtional mask to indicate which other gate bits in the same
 * register is mutually exclusive to this gate clock.
 */
struct clk_gate_exclusive {
	struct clk_hw hw;
	void __iomem *reg;
	int shift;
	const char *parent;
	u32 exclusive_mask;
};

static inline struct clk_gate_exclusive *to_clk_gate_exclusive(struct clk_hw *hw)
{
	return container_of(hw, struct clk_gate_exclusive, hw);
}

static int clk_gate_exclusive_enable(struct clk_hw *hw)
{
	struct clk_gate_exclusive *exgate = to_clk_gate_exclusive(hw);
	u32 val = readl(exgate->reg);

	if (val & exgate->exclusive_mask)
		return -EBUSY;

	val |= 1 << exgate->shift;

	writel(val, exgate->reg);

	return 0;
}

static void clk_gate_exclusive_disable(struct clk_hw *hw)
{
	struct clk_gate_exclusive *exgate = to_clk_gate_exclusive(hw);
	u32 val = readl(exgate->reg);

	val &= ~(1 << exgate->shift);

	writel(val, exgate->reg);
}

static int clk_gate_exclusive_is_enabled(struct clk_hw *hw)
{
	struct clk_gate_exclusive *exgate = to_clk_gate_exclusive(hw);

	return readl(exgate->reg) & (1 << exgate->shift);
}

static const struct clk_ops clk_gate_exclusive_ops = {
	.enable = clk_gate_exclusive_enable,
	.disable = clk_gate_exclusive_disable,
	.is_enabled = clk_gate_exclusive_is_enabled,
};

struct clk *imx_clk_gate_exclusive(const char *name, const char *parent,
	 void __iomem *reg, u8 shift, u32 exclusive_mask)
{
	struct clk_gate_exclusive *exgate;
	int ret;

	exgate = xzalloc(sizeof(*exgate));
	exgate->parent = parent;
	exgate->hw.clk.name = name;
	exgate->hw.clk.ops = &clk_gate_exclusive_ops;
	exgate->hw.clk.flags = CLK_SET_RATE_PARENT;
	exgate->hw.clk.parent_names = &exgate->parent;
	exgate->hw.clk.num_parents = 1;

	exgate->reg = reg;
	exgate->shift = shift;
	exgate->exclusive_mask = exclusive_mask;

	ret = bclk_register(&exgate->hw.clk);
	if (ret) {
		free(exgate);
		return ERR_PTR(ret);
	}

 	return &exgate->hw.clk;
}

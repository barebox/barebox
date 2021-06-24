// SPDX-License-Identifier: GPL-2.0
/*
 * Zynq UltraScale+ MPSoC Clock Gate
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

struct zynqmp_clk_gate {
	struct clk_hw hw;
	unsigned int clk_id;
	const char *parent;
	const struct zynqmp_eemi_ops *ops;
};

#define to_zynqmp_clk_gate(_hw) container_of(_hw, struct zynqmp_clk_gate, hw)

static int zynqmp_clk_gate_enable(struct clk_hw *hw)
{
	struct zynqmp_clk_gate *gate = to_zynqmp_clk_gate(hw);

	if (clk_hw_is_enabled(hw))
		return 0;

	return gate->ops->clock_enable(gate->clk_id);
}

static void zynqmp_clk_gate_disable(struct clk_hw *hw)
{
	struct zynqmp_clk_gate *gate = to_zynqmp_clk_gate(hw);

	gate->ops->clock_disable(gate->clk_id);
}

static int zynqmp_clk_gate_is_enabled(struct clk_hw *hw)
{
	struct zynqmp_clk_gate *gate = to_zynqmp_clk_gate(hw);
	u32 state;
	int ret;
	const struct zynqmp_eemi_ops *eemi_ops = zynqmp_pm_get_eemi_ops();

	ret = eemi_ops->clock_getstate(gate->clk_id, &state);
	if (ret)
		return -EIO;

	return !!state;
}

static const struct clk_ops zynqmp_clk_gate_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.enable = zynqmp_clk_gate_enable,
	.disable = zynqmp_clk_gate_disable,
	.is_enabled = zynqmp_clk_gate_is_enabled,
};

struct clk *zynqmp_clk_register_gate(const char *name,
				     unsigned int clk_id,
				     const char **parents,
				     unsigned int num_parents,
				     const struct clock_topology *nodes)
{
	struct zynqmp_clk_gate *gate;
	int ret;

	gate = kzalloc(sizeof(*gate), GFP_KERNEL);
	if (!gate)
		return ERR_PTR(-ENOMEM);

	gate->clk_id = clk_id;
	gate->ops = zynqmp_pm_get_eemi_ops();
	gate->parent = strdup(parents[0]);

	gate->hw.clk.name = strdup(name);
	gate->hw.clk.ops = &zynqmp_clk_gate_ops;
	gate->hw.clk.flags = nodes->flag | CLK_SET_RATE_PARENT;
	gate->hw.clk.parent_names = &gate->parent;
	gate->hw.clk.num_parents = 1;

	ret = bclk_register(&gate->hw.clk);
	if (ret) {
		kfree(gate);
		return ERR_PTR(ret);
	}

	return &gate->hw.clk;
}

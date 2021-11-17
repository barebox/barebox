// SPDX-License-Identifier: GPL-2.0-only
/*
 * Zynq UltraScale+ MPSoC Clock Multiplexer
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

#define ZYNQMP_CLK_MUX_READ_ONLY		BIT(3)

struct zynqmp_clk_mux {
	struct clk_hw hw;
	u32 clk_id;
	const struct zynqmp_eemi_ops *ops;
};

#define to_zynqmp_clk_mux(_hw) \
	container_of(_hw, struct zynqmp_clk_mux, hw)

static int zynqmp_clk_mux_get_parent(struct clk_hw *hw)
{
	struct zynqmp_clk_mux *mux = to_zynqmp_clk_mux(hw);
	u32 value;

	mux->ops->clock_getparent(mux->clk_id, &value);

	return value;
}

static int zynqmp_clk_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct zynqmp_clk_mux *mux = to_zynqmp_clk_mux(hw);

	return mux->ops->clock_setparent(mux->clk_id, index);
}

static const struct clk_ops zynqmp_clk_mux_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.get_parent = zynqmp_clk_mux_get_parent,
	.set_parent = zynqmp_clk_mux_set_parent,
};

static const struct clk_ops zynqmp_clk_mux_ro_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.get_parent = zynqmp_clk_mux_get_parent,
};

struct clk *zynqmp_clk_register_mux(const char *name,
				    unsigned int clk_id,
				    const char **parents,
				    unsigned int num_parents,
				    const struct clock_topology *nodes)
{
	struct zynqmp_clk_mux *mux;
	int ret;
	int i;
	const char **parent_names;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return ERR_PTR(-ENOMEM);

	parent_names = kcalloc(num_parents, sizeof(*parent_names), GFP_KERNEL);
	if (!parent_names) {
		kfree(mux);
		return ERR_PTR(-ENOMEM);
	}
	for (i = 0; i < num_parents; i++)
		parent_names[i] = strdup(parents[i]);

	mux->clk_id = clk_id;
	mux->ops = zynqmp_pm_get_eemi_ops();

	mux->hw.clk.name = strdup(name);
	if (nodes->type_flag & ZYNQMP_CLK_MUX_READ_ONLY)
		mux->hw.clk.ops = &zynqmp_clk_mux_ro_ops;
	else
		mux->hw.clk.ops = &zynqmp_clk_mux_ops;
	mux->hw.clk.flags = nodes->flag | CLK_SET_RATE_PARENT;
	mux->hw.clk.parent_names = parent_names;
	mux->hw.clk.num_parents = num_parents;

	ret = bclk_register(&mux->hw.clk);
	if (ret) {
		kfree(parent_names);
		kfree(mux);
		return ERR_PTR(ret);
	}

	return &mux->hw.clk;
}

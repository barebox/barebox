// SPDX-License-Identifier: GPL-2.0
/*
 * Zynq UltraScale+ MPSoC Clock Divider
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

struct zynqmp_clk_divider {
	struct clk clk;
	unsigned int clk_id;
	enum topology_type type;
	const char *parent;
	const struct zynqmp_eemi_ops *ops;
};
#define to_zynqmp_clk_divider(clk) \
	container_of(clk, struct zynqmp_clk_divider, clk)

static int zynqmp_clk_divider_bestdiv(unsigned long rate,
				      unsigned long *best_parent_rate)
{
	return DIV_ROUND_CLOSEST(*best_parent_rate, rate);
}

static unsigned long zynqmp_clk_divider_recalc_rate(struct clk *clk,
						    unsigned long parent_rate)
{
	struct zynqmp_clk_divider *div = to_zynqmp_clk_divider(clk);
	u32 value;

	div->ops->clock_getdivider(div->clk_id, &value);
	if (div->type == TYPE_DIV1)
		value = value & 0xFFFF;
	else
		value = value >> 16;

	return DIV_ROUND_UP(parent_rate, value);
}

static long zynqmp_clk_divider_round_rate(struct clk *clk, unsigned long rate,
					  unsigned long *parent_rate)
{
	int bestdiv;

	bestdiv = zynqmp_clk_divider_bestdiv(rate, parent_rate);

	return *parent_rate / bestdiv;
}

static int zynqmp_clk_divider_set_rate(struct clk *clk, unsigned long rate,
				       unsigned long parent_rate)
{
	struct zynqmp_clk_divider *div = to_zynqmp_clk_divider(clk);
	u32 bestdiv;

	bestdiv = zynqmp_clk_divider_bestdiv(rate, &parent_rate);
	if (div->type == TYPE_DIV1)
		bestdiv = (0xffff << 16) | (bestdiv & 0xffff);
	else
		bestdiv = (bestdiv << 16) | 0xffff;

	return div->ops->clock_setdivider(div->clk_id, bestdiv);
}

static const struct clk_ops zynqmp_clk_divider_ops = {
	.recalc_rate = zynqmp_clk_divider_recalc_rate,
	.round_rate = zynqmp_clk_divider_round_rate,
	.set_rate = zynqmp_clk_divider_set_rate,
};

struct clk *zynqmp_clk_register_divider(const char *name,
					unsigned int clk_id,
					const char **parents,
					unsigned int num_parents,
					const struct clock_topology *nodes)
{
	struct zynqmp_clk_divider *div;
	int ret;

	div = kzalloc(sizeof(*div), GFP_KERNEL);
	if (!div)
		return ERR_PTR(-ENOMEM);

	div->clk_id = clk_id;
	div->type = nodes->type;
	div->ops = zynqmp_pm_get_eemi_ops();
	div->parent = strdup(parents[0]);

	div->clk.name = strdup(name);
	div->clk.ops = &zynqmp_clk_divider_ops;
	div->clk.flags = nodes->flag;
	div->clk.parent_names = &div->parent;
	div->clk.num_parents = 1;

	ret = clk_register(&div->clk);
	if (ret) {
		kfree(div);
		return ERR_PTR(ret);
	}

	return &div->clk;
}

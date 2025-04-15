// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Collabora Ltd.
 * Author: Sebastian Reichel <sebastian.reichel@collabora.com>
 */

#include <common.h>

#include "clk.h"

static int rk_clk_gate_link_register(struct device *dev,
				     struct rockchip_clk_provider *ctx,
				     struct rockchip_clk_branch *clkbr)
{
	unsigned long flags = clkbr->flags | CLK_SET_RATE_PARENT;
	struct clk *clk;

	clk = clk_register_gate(dev, clkbr->name, clkbr->parent_names[0],
				flags, ctx->reg_base + clkbr->gate_offset,
				clkbr->gate_shift, clkbr->gate_flags,
				&ctx->lock);

	if (IS_ERR(clk))
		return PTR_ERR(clk);

	rockchip_clk_set_lookup(ctx, clk, clkbr->id);

	return 0;
}

static int rk_clk_gate_link_probe(struct device *dev)
{
	struct rockchip_gate_link_platdata *pdata = dev->platform_data;
	if (!pdata)
		return dev_err_probe(dev, -ENODEV, "missing platform data");

	return rk_clk_gate_link_register(dev, pdata->ctx, pdata->clkbr);
}

static struct driver rk_clk_gate_link_driver = {
	.probe	= rk_clk_gate_link_probe,
	.name	= "rockchip-gate-link-clk",
};
core_platform_driver(rk_clk_gate_link_driver);

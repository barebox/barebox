/*
 * Marvell EBU SoC common clock handling
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux Marvell MVEBU clock providers
 *   Copyright (C) 2012 Marvell
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <init.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clkdev.h>

#include "common.h"

/*
 * Core Clocks
 */

static struct clk_onecell_data clk_data;

static struct of_device_id mvebu_coreclk_ids[] = {
	{ .compatible = "marvell,armada-370-core-clock",
	  .data = &armada_370_coreclks },
	{ .compatible = "marvell,armada-380-core-clock",
	  .data = &armada_38x_coreclks },
	{ .compatible = "marvell,armada-xp-core-clock",
	  .data = &armada_xp_coreclks },
	{ .compatible = "marvell,dove-core-clock",
	  .data = &dove_coreclks },
	{ .compatible = "marvell,kirkwood-core-clock",
	  .data = &kirkwood_coreclks },
	{ .compatible = "marvell,mv88f6180-core-clock",
	  .data = &mv88f6180_coreclks },
	{ }
};

int mvebu_coreclk_probe(struct device_d *dev)
{
	struct resource *iores;
	struct device_node *np = dev->device_node;
	const struct of_device_id *match;
	const struct coreclk_soc_desc *desc;
	const char *tclk_name = "tclk";
	const char *cpuclk_name = "cpuclk";
	void __iomem *base;
	unsigned long rate;
	int n;

	match = of_match_node(mvebu_coreclk_ids, np);
	if (!match)
		return -EINVAL;
	desc = (const struct coreclk_soc_desc *)match->data;

	/* Get SAR base address */
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	/* Allocate struct for TCLK, cpu clk, and core ratio clocks */
	clk_data.clk_num = 2 + desc->num_ratios;
	clk_data.clks = xzalloc(clk_data.clk_num * sizeof(struct clk *));

	/* Register TCLK */
	of_property_read_string_index(np, "clock-output-names", 0,
				      &tclk_name);
	rate = desc->get_tclk_freq(base);
	clk_data.clks[0] = clk_fixed(tclk_name, rate);
	WARN_ON(IS_ERR(clk_data.clks[0]));

	/* Register CPU clock */
	of_property_read_string_index(np, "clock-output-names", 1,
				      &cpuclk_name);
	rate = desc->get_cpu_freq(base);
	clk_data.clks[1] = clk_fixed(cpuclk_name, rate);
	WARN_ON(IS_ERR(clk_data.clks[1]));

	/* Register fixed-factor clocks derived from CPU clock */
	for (n = 0; n < desc->num_ratios; n++) {
		const char *rclk_name = desc->ratios[n].name;
		int mult, div;

		of_property_read_string_index(np, "clock-output-names",
					      2+n, &rclk_name);
		desc->get_clk_ratio(base, desc->ratios[n].id, &mult, &div);
		clk_data.clks[2+n] = clk_fixed_factor(rclk_name, cpuclk_name,
						mult, div, 0);
		WARN_ON(IS_ERR(clk_data.clks[2+n]));
	};

	return of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
}

static struct driver_d mvebu_coreclk_driver = {
	.probe	= mvebu_coreclk_probe,
	.name	= "mvebu-core-clk",
	.of_compatible = DRV_OF_COMPAT(mvebu_coreclk_ids),
};

static int mvebu_coreclk_init(void)
{
	return platform_driver_register(&mvebu_coreclk_driver);
}
core_initcall(mvebu_coreclk_init);

/*
 * Clock Gating Control
 */

struct gate {
	struct clk *clk;
	int bit_idx;
};

struct clk_gating_ctrl {
	struct gate *gates;
	int num_gates;
};

static struct clk *clk_gating_get_src(
	struct of_phandle_args *clkspec, void *data)
{
	struct clk_gating_ctrl *ctrl = (struct clk_gating_ctrl *)data;
	int n;

	if (clkspec->args_count < 1)
		return ERR_PTR(-EINVAL);

	for (n = 0; n < ctrl->num_gates; n++) {
		if (clkspec->args[0] == ctrl->gates[n].bit_idx)
			return ctrl->gates[n].clk;
	}
	return ERR_PTR(-ENODEV);
}

static struct of_device_id mvebu_clk_gating_ids[] = {
	{ .compatible = "marvell,armada-370-gating-clock",
	  .data = &armada_370_gating_desc },
	{ .compatible = "marvell,armada-xp-gating-clock",
	  .data = &armada_xp_gating_desc },
	{ .compatible = "marvell,armada-380-gating-clock",
	  .data = &armada_38x_gating_desc },
	{ .compatible = "marvell,dove-gating-clock",
	  .data = &dove_gating_desc },
	{ .compatible = "marvell,kirkwood-gating-clock",
	  .data = &kirkwood_gating_desc },
	{ }
};

int mvebu_clk_gating_probe(struct device_d *dev)
{
	struct resource *iores;
	struct device_node *np = dev->device_node;
	const struct of_device_id *match;
	const struct clk_gating_soc_desc *desc;
	struct clk_gating_ctrl *ctrl;
	struct gate *gate;
	struct clk *clk;
	void __iomem *base;
	const char *default_parent = NULL;
	int n;

	match = of_match_node(mvebu_clk_gating_ids, np);
	if (!match)
		return -EINVAL;
	desc = (const struct clk_gating_soc_desc *)match->data;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	clk = of_clk_get(np, 0);
	if (IS_ERR(clk))
		return -EINVAL;

	default_parent = clk->name;
	ctrl = xzalloc(sizeof(*ctrl));

	/* Count, allocate, and register clock gates */
	for (n = 0; desc[n].name;)
		n++;

	ctrl->num_gates = n;
	ctrl->gates = xzalloc(ctrl->num_gates * sizeof(*gate));

	for (n = 0, gate = ctrl->gates; n < ctrl->num_gates; n++, gate++) {
		const char *parent =
			(desc[n].parent) ? desc[n].parent : default_parent;
		gate->bit_idx = desc[n].bit_idx;
		gate->clk = clk_gate(desc[n].name, parent,
				base, desc[n].bit_idx, 0, 0);
		WARN_ON(IS_ERR(gate->clk));
	}

	return of_clk_add_provider(np, clk_gating_get_src, ctrl);
}

static struct driver_d mvebu_clk_gating_driver = {
	.probe	= mvebu_clk_gating_probe,
	.name	= "mvebu-clk-gating",
	.of_compatible = DRV_OF_COMPAT(mvebu_clk_gating_ids),
};

static int mvebu_clk_gating_init(void)
{
	return platform_driver_register(&mvebu_clk_gating_driver);
}
postcore_initcall(mvebu_clk_gating_init);

// SPDX-License-Identifier: GPL-2.0
/*
 * Simple Power-Managed Bus Driver
 *
 * Copyright (C) 2014-2015 Glider bvba
 */

#include <linux/clk.h>
#include <driver.h>
#include <of.h>

static int simple_pm_bus_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct clk_bulk_data *clks;
	int num_clks;

	if (deep_probe_is_supported()) {
		num_clks = clk_bulk_get_all(dev, &clks);
		if (num_clks < 0)
			return dev_err_probe(dev, num_clks, "failed to get clocks\n");

		num_clks = clk_bulk_prepare_enable(num_clks, clks);
		if (num_clks)
			return dev_err_probe(dev, num_clks, "failed to enable clocks\n");
	}

	of_platform_populate(np, NULL, dev);

	return 0;
}

static const struct of_device_id simple_pm_bus_of_match[] = {
	{ .compatible = "simple-pm-bus", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, simple_pm_bus_of_match);

static struct driver simple_pm_bus_driver = {
	.probe = simple_pm_bus_probe,
	.name = "simple-pm-bus",
	.of_match_table = simple_pm_bus_of_match,
};
core_platform_driver(simple_pm_bus_driver);

MODULE_DESCRIPTION("Simple Power-Managed Bus Driver");
MODULE_AUTHOR("Geert Uytterhoeven <geert+renesas@glider.be>");

// SPDX-License-Identifier: GPL-2.0-only
/*
 * dwc3-generic-plat.c - DesignWare USB3 generic platform driver
 *
 * Copyright (C) 2025 Ze Huang <huang.ze@linux.dev>
 *
 * Inspired by dwc3-qcom.c and dwc3-of-simple.c
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <mfd/syscon.h>
#include "of_device.h"
#include "glue.h"

struct dwc3_generic {
	struct device		*dev;
	struct dwc3		dwc;
	struct clk_bulk_data	*clks;
	int			num_clocks;
	struct reset_control	*resets;
};

struct dwc3_generic_config {
	int (*init)(struct dwc3_generic *dwc3g);
	struct dwc3_properties properties;
};

#define to_dwc3_generic(d) container_of((d), struct dwc3_generic, dwc)

static int dwc3_generic_probe(struct device *dev)
{
	const struct dwc3_generic_config *plat_config;
	struct dwc3_probe_data probe_data = {};
	struct dwc3_generic *dwc3g;
	struct resource *res;
	int ret;

	dwc3g = xzalloc(sizeof(*dwc3g));

	dwc3g->dev = dev;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing memory resource\n");
		return -ENODEV;
	}

	dwc3g->resets = reset_control_array_get(dev);
	if (IS_ERR(dwc3g->resets))
		return dev_err_probe(dev, PTR_ERR(dwc3g->resets), "failed to get resets\n");

	ret = reset_control_assert(dwc3g->resets);
	if (ret)
		return dev_err_probe(dev, ret, "failed to assert resets\n");

	/* Not strict timing, just for safety */
	udelay(2);

	ret = reset_control_deassert(dwc3g->resets);
	if (ret)
		return dev_err_probe(dev, ret, "failed to deassert resets\n");

	ret = clk_bulk_get_all_enabled(dwc3g->dev, &dwc3g->clks);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to get clocks\n");

	dwc3g->num_clocks = ret;
	dwc3g->dwc.dev = dev;
	probe_data.dwc = &dwc3g->dwc;
	probe_data.res = res;
	probe_data.ignore_clocks_and_resets = true;

	plat_config = of_device_get_match_data(dev);
	if (!plat_config) {
		probe_data.properties = DWC3_DEFAULT_PROPERTIES;
		goto core_probe;
	}

	probe_data.properties = plat_config->properties;
	if (plat_config->init) {
		ret = plat_config->init(dwc3g);
		if (ret)
			return dev_err_probe(dev, ret,
					     "failed to init platform\n");
	}

core_probe:
	ret = dwc3_core_probe(&probe_data);
	if (ret)
		return dev_err_probe(dev, ret, "failed to register DWC3 Core\n");

	return 0;
}

static void dwc3_generic_remove(struct device *dev)
{
	struct dwc3 *dwc = dev_get_drvdata(dev);

	dwc3_core_remove(dwc);
}

static const struct dwc3_generic_config fsl_ls1028_dwc3 = {
	.properties.gsbuscfg0_reqinfo = 0x2222,
};

static const struct of_device_id dwc3_generic_of_match[] = {
	{ .compatible = "spacemit,k1-dwc3", },
	{ .compatible = "fsl,ls1028a-dwc3", &fsl_ls1028_dwc3},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, dwc3_generic_of_match);

static struct driver dwc3_generic_driver = {
	.probe		= dwc3_generic_probe,
	.remove		= dwc3_generic_remove,
	.name	= "dwc3-generic-plat",
	.of_match_table = dwc3_generic_of_match,
};
device_platform_driver(dwc3_generic_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DesignWare USB3 generic platform driver");

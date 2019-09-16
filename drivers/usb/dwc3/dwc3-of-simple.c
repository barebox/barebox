// SPDX-License-Identifier: GPL-2.0
/**
 * dwc3-of-simple.c - OF glue layer for simple integrations
 *
 * Copyright (c) 2015 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * This is a combination of the old dwc3-qcom.c by Ivan T. Ivanov
 * <iivanov@mm-sol.com> and the original patch adding support for Xilinx' SoC
 * by Subbaraya Sundeep Bhatta <subbaraya.sundeep.bhatta@xilinx.com>
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <of.h>

struct dwc3_of_simple {
	struct device_d		*dev;
	struct clk		**clks;
	int			num_clocks;
};

static int dwc3_of_simple_clk_init(struct dwc3_of_simple *simple, int count)
{
	struct device_d		*dev = simple->dev;
	struct device_node	*np = dev->device_node;
	int			i;

	simple->num_clocks = count;

	if (!count)
		return 0;

	simple->clks = xzalloc(sizeof(struct clk *));
	if (!simple->clks)
		return -ENOMEM;

	for (i = 0; i < simple->num_clocks; i++) {
		struct clk	*clk;

		clk = of_clk_get(np, i);
		if (IS_ERR(clk)) {
			while (--i >= 0) {
				clk_disable(simple->clks[i]);
				clk_put(simple->clks[i]);
			}
			return PTR_ERR(clk);
		}

		simple->clks[i] = clk;
	}

	return 0;
}

static int dwc3_of_simple_probe(struct device_d *dev)
{
	struct dwc3_of_simple	*simple;
	struct device_node	*np = dev->device_node;

	int			ret;
	int			i;

	simple = xzalloc(sizeof(*simple));
	if (!simple)
		return -ENOMEM;

        dev->priv = simple;
	simple->dev = dev;

	ret = dwc3_of_simple_clk_init(simple, of_count_phandle_with_args(np,
						"clocks", "#clock-cells"));
	if (ret)
		return ret;

        ret = of_platform_populate(np, NULL, dev);
	if (ret) {
		for (i = 0; i < simple->num_clocks; i++) {
			clk_disable(simple->clks[i]);
			clk_put(simple->clks[i]);
		}
		return ret;
	}

        return 0;
}

static void dwc3_of_simple_remove(struct device_d *dev)
{
	struct dwc3_of_simple	*simple = dev->priv;
	int			i;

	for (i = 0; i < simple->num_clocks; i++) {
		clk_disable(simple->clks[i]);
		clk_put(simple->clks[i]);
	}
	simple->num_clocks = 0;
}

static const struct of_device_id of_dwc3_simple_match[] = {
	{.compatible = "rockchip,rk3399-dwc3"},
	{.compatible = "xlnx,zynqmp-dwc3"},
	{.compatible = "fsl,ls1046a-dwc3"},
	{.compatible = "cavium,octeon-7130-usb-uctl"},
	{.compatible = "sprd,sc9860-dwc3"},
	{.compatible = "amlogic,meson-axg-dwc3"},
	{.compatible = "amlogic,meson-gxl-dwc3"},
	{.compatible = "allwinner,sun50i-h6-dwc3"},
	{/* Sentinel */}};

static struct driver_d dwc3_of_simple_driver = {
	.probe		= dwc3_of_simple_probe,
	.remove		= dwc3_of_simple_remove,
	.name		= "dwc3-of-simple",
	.of_compatible	= DRV_OF_COMPAT(of_dwc3_simple_match),
};
device_platform_driver(dwc3_of_simple_driver);

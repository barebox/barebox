// SPDX-License-Identifier: GPL-2.0-only
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
	struct device		*dev;
	struct clk_bulk_data	*clks;
	int			num_clocks;
};

static int dwc3_of_simple_probe(struct device *dev)
{
	struct dwc3_of_simple	*simple;
	struct device_node	*np = dev->of_node;

	int			ret;

	simple = xzalloc(sizeof(*simple));
	if (!simple)
		return -ENOMEM;

	dev->priv = simple;
	simple->dev = dev;

	ret = clk_bulk_get_all(simple->dev, &simple->clks);
	if (ret < 0)
		return ret;

	simple->num_clocks = ret;
	ret = clk_bulk_enable(simple->num_clocks, simple->clks);
	if (ret)
		return ret;

        ret = of_platform_populate(np, NULL, dev);
	if (ret) {
		clk_bulk_disable(simple->num_clocks, simple->clks);
		return ret;
	}

	return 0;
}

static void dwc3_of_simple_remove(struct device *dev)
{
	struct dwc3_of_simple	*simple = dev->priv;

	clk_bulk_disable(simple->num_clocks, simple->clks);
}

static const struct of_device_id of_dwc3_simple_match[] = {
	{.compatible = "rockchip,rk3399-dwc3"},
	{.compatible = "xlnx,zynqmp-dwc3"},
	{.compatible = "fsl,ls1046a-dwc3"},
	{.compatible = "fsl,imx8mp-dwc3"},
	{.compatible = "cavium,octeon-7130-usb-uctl"},
	{.compatible = "sprd,sc9860-dwc3"},
	{.compatible = "amlogic,meson-axg-dwc3"},
	{.compatible = "amlogic,meson-gxl-dwc3"},
	{.compatible = "allwinner,sun50i-h6-dwc3"},
	{/* Sentinel */}};
MODULE_DEVICE_TABLE(of, of_dwc3_simple_match);

static struct driver dwc3_of_simple_driver = {
	.probe		= dwc3_of_simple_probe,
	.remove		= dwc3_of_simple_remove,
	.name		= "dwc3-of-simple",
	.of_compatible	= DRV_OF_COMPAT(of_dwc3_simple_match),
};
device_platform_driver(dwc3_of_simple_driver);

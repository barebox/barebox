// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2022 NXP
 */

#include <init.h>
#include <driver.h>
#include <linux/module.h>

static int imx93_src_probe(struct device *dev)
{
	return of_platform_populate(dev->of_node, NULL, dev);
}

static const struct of_device_id imx93_src_ids[] = {
	{ .compatible = "fsl,imx93-src" },
	{ }
};
MODULE_DEVICE_TABLE(of, imx93_src_ids);

static struct driver imx93_src_driver = {
	.name	= "imx93_src",
	.probe = imx93_src_probe,
	.of_compatible = imx93_src_ids,
};
core_platform_driver(imx93_src_driver);

MODULE_AUTHOR("Peng Fan <peng.fan@nxp.com>");
MODULE_DESCRIPTION("NXP i.MX93 src driver");
MODULE_LICENSE("GPL");

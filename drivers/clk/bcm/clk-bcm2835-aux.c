// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015 Broadcom
 */

#include <linux/clk.h>
#include <io.h>
#include <of_address.h>
#include <driver.h>
#include <init.h>
#include <dt-bindings/clock/bcm2835-aux.h>

#define BCM2835_AUXIRQ		0x00
#define BCM2835_AUXENB		0x04

static int bcm2835_aux_clk_probe(struct device *dev)
{
	struct clk_hw_onecell_data *onecell;
	const char *parent;
	struct clk *parent_clk;
	void __iomem *reg, *gate;

	parent_clk = clk_get(dev, NULL);
	if (IS_ERR(parent_clk))
		return PTR_ERR(parent_clk);
	parent = __clk_get_name(parent_clk);

	reg = of_iomap(dev->of_node, 0);
	if (!reg)
		return -ENOMEM;

	onecell = kmalloc(struct_size(onecell, hws, BCM2835_AUX_CLOCK_COUNT),
			  GFP_KERNEL);
	if (!onecell)
		return -ENOMEM;
	onecell->num = BCM2835_AUX_CLOCK_COUNT;

	gate = reg + BCM2835_AUXENB;
	onecell->hws[BCM2835_AUX_CLOCK_UART] =
		clk_hw_register_gate(dev, "aux_uart", parent, 0, gate, 0, 0, NULL);

	onecell->hws[BCM2835_AUX_CLOCK_SPI1] =
		clk_hw_register_gate(dev, "aux_spi1", parent, 0, gate, 1, 0, NULL);

	onecell->hws[BCM2835_AUX_CLOCK_SPI2] =
		clk_hw_register_gate(dev, "aux_spi2", parent, 0, gate, 2, 0, NULL);

	return of_clk_add_hw_provider(dev->of_node, of_clk_hw_onecell_get,
				      onecell);
}

static const struct of_device_id bcm2835_aux_clk_of_match[] = {
	{ .compatible = "brcm,bcm2835-aux", },
	{},
};
MODULE_DEVICE_TABLE(of, bcm2835_aux_clk_of_match);

static struct driver bcm2835_aux_clk_driver = {
	.name = "bcm2835-aux-clk",
	.of_compatible = bcm2835_aux_clk_of_match,
	.probe          = bcm2835_aux_clk_probe,
};
core_platform_driver(bcm2835_aux_clk_driver);

MODULE_AUTHOR("Eric Anholt <eric@anholt.net>");
MODULE_DESCRIPTION("BCM2835 auxiliary peripheral clock driver");
MODULE_LICENSE("GPL");

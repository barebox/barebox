/*
 * (C) Copyright 2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <linux/clk.h>
#include <driver.h>
#include <init.h>
#include <usb/usb.h>
#include <usb/usb_defs.h>
#include <errno.h>
#include <io.h>

#include "ohci.h"

/* interface and function clocks; sometimes also an AHB clock */
static struct clk *iclk, *fclk;

static void at91_start_clock(void)
{
	clk_enable(iclk);
	clk_enable(fclk);
}

static void at91_stop_clock(void)
{
	clk_disable(fclk);
	clk_disable(iclk);
}

static int at91_ohci_probe(struct device_d *dev)
{
	struct ohci_regs __iomem *regs = (struct ohci_regs __iomem *)dev->resource[0].start;

	iclk = clk_get(NULL, "ohci_clk");
	fclk = clk_get(NULL, "uhpck");

	/*
	 * Start the USB clocks.
	 */
	at91_start_clock();

	/*
	 * The USB host controller must remain in reset.
	 */
	writel(0, &regs->control);

	add_generic_device("ohci", DEVICE_ID_DYNAMIC, NULL, dev->resource[0].start,
			   resource_size(&dev->resource[0]), IORESOURCE_MEM, NULL);

	return 0;
}

static void at91_ohci_remove(struct device_d *dev)
{
	struct ohci_regs __iomem *regs = (struct ohci_regs __iomem *)dev->resource[0].start;

	/*
	 * Put the USB host controller into reset.
	 */
	writel(0, &regs->control);

	/*
	 * Stop the USB clocks.
	 */
	at91_stop_clock();
}

static struct driver_d at91_ohci_driver = {
	.name = "at91_ohci",
	.probe = at91_ohci_probe,
	.remove = at91_ohci_remove,
};
device_platform_driver(at91_ohci_driver);

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
#include <linux/err.h>
#include <driver.h>
#include <init.h>
#include <usb/usb.h>
#include <usb/usb_defs.h>
#include <usb/ehci.h>
#include <errno.h>
#include <io.h>

#include "ehci.h"

/* interface and function clocks; sometimes also an AHB clock */
static struct clk *iclk, *fclk;

static void atmel_start_clock(void)
{
	clk_enable(iclk);
	clk_enable(fclk);
}

static void atmel_stop_clock(void)
{
	clk_disable(fclk);
	clk_disable(iclk);
}

static int atmel_ehci_probe(struct device_d *dev)
{
	struct ehci_data data;

	iclk = clk_get(dev, "ehci_clk");
	if (IS_ERR(iclk)) {
		dev_err(dev, "Error getting interface clock\n");
		return -ENOENT;
	}

	fclk = clk_get(dev, "uhpck");
	if (IS_ERR(fclk)) {
		dev_err(dev, "Error getting function clock\n");
		return -ENOENT;
	}

	/*
	 * Start the USB clocks.
	 */
	atmel_start_clock();

	data.flags = 0;

	data.hccr = dev_request_mem_region(dev, 0);

	ehci_register(dev, &data);

	return 0;
}

static void atmel_ehci_remove(struct device_d *dev)
{
	/*
	 * Stop the USB clocks.
	 */
	atmel_stop_clock();
}

static struct driver_d atmel_ehci_driver = {
	.name = "atmel-ehci",
	.probe = atmel_ehci_probe,
	.remove = atmel_ehci_remove,
};
device_platform_driver(atmel_ehci_driver);

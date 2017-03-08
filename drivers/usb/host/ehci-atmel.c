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

struct atmel_ehci_priv {
	struct device_d *dev;
	struct clk *iclk;
	struct clk *uclk;
};

static int atmel_start_clock(struct atmel_ehci_priv *atehci)
{
	int ret;
	ret = clk_enable(atehci->iclk);
	if (ret < 0) {
		dev_err(atehci->dev,
			"Error enabling interface clock\n");
		return ret;
	}

	ret = clk_enable(atehci->uclk);
	if (ret < 0)
		dev_err(atehci->dev,
			"Error enabling function clock\n");

	return ret;
}

static void atmel_stop_clock(struct atmel_ehci_priv *atehci)
{
	clk_disable(atehci->iclk);
	clk_disable(atehci->uclk);
}

static int atmel_ehci_probe(struct device_d *dev)
{
	int ret;
	struct resource *iores;
	struct ehci_data data;
	struct atmel_ehci_priv *atehci;

	atehci = xzalloc(sizeof(*atehci));
	atehci->dev = dev;
	dev->priv = atehci;

	atehci->iclk = clk_get(dev, "ehci_clk");
	if (IS_ERR(atehci->iclk)) {
		dev_err(dev, "Error getting interface clock\n");
		return -ENOENT;
	}

	atehci->uclk = clk_get(dev, "uhpck");
	if (IS_ERR(atehci->iclk)) {
		dev_err(dev, "Error getting function clock\n");
		return -ENOENT;
	}

	/*
	 * Start the USB clocks.
	 */
	ret = atmel_start_clock(atehci);
	if (ret < 0)
		return ret;

	memset(&data, 0, sizeof(data));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	data.hccr = IOMEM(iores->start);

	return ehci_register(dev, &data);
}

static void atmel_ehci_remove(struct device_d *dev)
{
	/*
	 * Stop the USB clocks.
	 */
	atmel_stop_clock(dev->priv);
}

static struct driver_d atmel_ehci_driver = {
	.name = "atmel-ehci",
	.probe = atmel_ehci_probe,
	.remove = atmel_ehci_remove,
};
device_platform_driver(atmel_ehci_driver);

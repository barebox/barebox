// SPDX-License-Identifier: GPL-2.0
/**
 * host.c - DesignWare USB3 DRD Controller Host Glue
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 */

#include <common.h>
#include <driver.h>
#include <init.h>

#include "core.h"

int dwc3_host_init(struct dwc3 *dwc)
{
	struct resource *io;
	struct device_d *dev = dwc->dev;

	io = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(io)) {
		dev_err(dev, "Failed to get IORESOURCE_MEM\n");
		return PTR_ERR(io);
	}

	dwc->xhci = add_generic_device("xHCI", DEVICE_ID_DYNAMIC, NULL,
				       io->start, resource_size(io),
				       IORESOURCE_MEM, NULL);
	if (!dwc->xhci) {
		dev_err(dev, "Failed to register xHCI device\n");
		return -ENODEV;
	}
	
	return 0;
}

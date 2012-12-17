/*
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <driver.h>
#include <usb/ehci.h>
#include <usb/chipidea-imx.h>
#include <usb/ulpi.h>

#define MXC_EHCI_PORTSC_MASK (0xf << 28)

static int imx_chipidea_probe(struct device_d *dev)
{
	struct imxusb_platformdata *pdata = dev->platform_data;
	int ret;
	void __iomem *base;
	struct ehci_data data;
	uint32_t portsc;

	if (!pdata) {
		dev_err(dev, "no pdata!\n");
		return -EINVAL;
	}

	base = dev_request_mem_region(dev, 0);
	if (!base)
		return -ENODEV;

	portsc = readl(base + 0x184);
	portsc &= ~MXC_EHCI_PORTSC_MASK;
	portsc |= pdata->flags & MXC_EHCI_PORTSC_MASK;
	writel(portsc, base + 0x184);

	ret = imx_usbmisc_port_init(dev->id, pdata->flags);
	if (ret) {
		dev_err(dev, "failed to init misc regs: %s\n", strerror(-ret));
		return ret;
	}

	if ((pdata->flags & MXC_EHCI_PORTSC_MASK) == MXC_EHCI_MODE_ULPI) {
		dev_dbg(dev, "using ULPI phy\n");
		if (IS_ENABLED(CONFIG_USB_ULPI)) {
			ret = ulpi_setup(base + 0x170, 1);
		} else {
			dev_err(dev, "no ULPI support available\n");
			ret = -ENODEV;
		}

		if (ret)
			return ret;
	}

	data.hccr = base + 0x100;
	data.hcor = base + 0x140;
	data.flags = EHCI_HAS_TT;

	if (pdata->mode == IMX_USB_MODE_HOST) {
		ret = ehci_register(dev, &data);
	} else {
		/*
		 * Not yet implemented. Register USB gadget driver here.
		 */
		ret = -ENOSYS;
	}

	return ret;
};

static struct driver_d imx_chipidea_driver = {
	.name   = "imx-usb",
	.probe  = imx_chipidea_probe,
};

static int imx_chipidea_init(void)
{
	return platform_driver_register(&imx_chipidea_driver);
}
device_initcall(imx_chipidea_init);

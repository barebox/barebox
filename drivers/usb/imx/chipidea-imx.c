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
#include <usb/usb.h>
#include <usb/ehci.h>
#include <usb/chipidea-imx.h>
#include <usb/ulpi.h>
#include <usb/fsl_usb2.h>

#define MXC_EHCI_PORTSC_MASK ((0xf << 28) | (1 << 25))

struct imx_chipidea {
	struct device_d *dev;
	struct ehci_data data;
	unsigned long flags;
	enum imx_usb_mode mode;
	int (*init)(int port);
	int portno;
};

static int imx_chipidea_port_init(void *drvdata)
{
	struct imx_chipidea *ci = drvdata;
	int ret;

	ret = imx_usbmisc_port_init(ci->portno, ci->flags);
	if (ret)
		dev_err(ci->dev, "misc init failed: %s\n", strerror(-ret));

	if (ci->init)
		ci->init(ci->portno);

	return ret;
}

static int imx_chipidea_port_post_init(void *drvdata)
{
	struct imx_chipidea *ci = drvdata;
	int ret;

	ret = imx_usbmisc_port_post_init(ci->portno, ci->flags);
	if (ret)
		dev_err(ci->dev, "post misc init failed: %s\n", strerror(-ret));

	return ret;
}

static int imx_chipidea_probe_dt(struct imx_chipidea *ci)
{
	const void *out_args;
	struct device_node *usbmisc_np;
	enum usb_dr_mode mode;
	enum usb_phy_interface phymode;

	of_parse_phandles_with_args(ci->dev->device_node, "fsl,usbmisc",
			"#index-cells", 0, &usbmisc_np, &out_args);

	ci->portno = be32_to_cpup(out_args);

	ci->flags = MXC_EHCI_MODE_UTMI_8BIT;

	mode = of_usb_get_dr_mode(ci->dev->device_node, NULL);

	switch (mode) {
	case USB_DR_MODE_HOST:
	default:
		ci->mode = IMX_USB_MODE_HOST;
		break;
	case USB_DR_MODE_PERIPHERAL:
		ci->mode = IMX_USB_MODE_DEVICE;
		break;
	}

	phymode = of_usb_get_phy_mode(ci->dev->device_node, NULL);
	switch (phymode) {
	case USBPHY_INTERFACE_MODE_UTMI:
		ci->flags = MXC_EHCI_MODE_UTMI_8BIT;
		break;
	case USBPHY_INTERFACE_MODE_UTMIW:
		ci->flags = MXC_EHCI_MODE_UTMI_16_BIT;
		break;
	case USBPHY_INTERFACE_MODE_ULPI:
		ci->flags = MXC_EHCI_MODE_ULPI;
		break;
	case USBPHY_INTERFACE_MODE_SERIAL:
		ci->flags = MXC_EHCI_MODE_SERIAL;
		break;
	case USBPHY_INTERFACE_MODE_HSIC:
		ci->flags = MXC_EHCI_MODE_HSIC;
		break;
	default:
		dev_err(ci->dev, "no or invalid phy mode setting\n");
		return -EINVAL;
	}

	return 0;
}

static int imx_chipidea_probe(struct device_d *dev)
{
	struct imxusb_platformdata *pdata = dev->platform_data;
	int ret;
	void __iomem *base;
	struct imx_chipidea *ci;
	uint32_t portsc;

	ci = xzalloc(sizeof(*ci));
	ci->dev = dev;

	if (IS_ENABLED(CONFIG_OFDEVICE) && dev->device_node) {
		ret = imx_chipidea_probe_dt(ci);
		if (ret)
			return ret;
	} else {
		if (!pdata) {
			dev_err(dev, "no pdata!\n");
			return -EINVAL;
		}
		ci->portno = dev->id;
		ci->flags = pdata->flags;
		ci->init = pdata->init;
		ci->mode = pdata->mode;
	}

	base = dev_request_mem_region(dev, 0);
	if (!base)
		return -ENODEV;

	ci->data.init = imx_chipidea_port_init;
	ci->data.post_init = imx_chipidea_port_post_init;
	ci->data.drvdata = ci;

	portsc = readl(base + 0x184);
	portsc &= ~MXC_EHCI_PORTSC_MASK;
	portsc |= ci->flags & MXC_EHCI_PORTSC_MASK;
	writel(portsc, base + 0x184);

	imx_chipidea_port_init(ci);

	if ((ci->flags & MXC_EHCI_PORTSC_MASK) == MXC_EHCI_MODE_ULPI) {
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

	ci->data.hccr = base + 0x100;
	ci->data.hcor = base + 0x140;
	ci->data.flags = EHCI_HAS_TT;

	if (ci->mode == IMX_USB_MODE_HOST && IS_ENABLED(CONFIG_USB_EHCI)) {
		ret = ehci_register(dev, &ci->data);
	} else if (ci->mode == IMX_USB_MODE_DEVICE && IS_ENABLED(CONFIG_USB_GADGET_DRIVER_ARC)) {
		ret = ci_udc_register(dev, base);
	} else {
		dev_err(dev, "No supported role\n");
		ret = -ENODEV;
	}

	return ret;
};

static __maybe_unused struct of_device_id imx_chipidea_dt_ids[] = {
	{
		.compatible = "fsl,imx27-usb",
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_chipidea_driver = {
	.name   = "imx-usb",
	.probe  = imx_chipidea_probe,
	.of_compatible = DRV_OF_COMPAT(imx_chipidea_dt_ids),
};
device_platform_driver(imx_chipidea_driver);

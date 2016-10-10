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
#include <of.h>
#include <errno.h>
#include <driver.h>
#include <usb/usb.h>
#include <usb/ehci.h>
#include <regulator.h>
#include <usb/chipidea-imx.h>
#include <usb/phy.h>
#include <usb/ulpi.h>
#include <usb/fsl_usb2.h>
#include <linux/err.h>
#include <linux/phy/phy.h>

#define MXC_EHCI_PORTSC_MASK ((0xf << 28) | (1 << 25))

struct imx_chipidea {
	struct device_d *dev;
	void __iomem *base;
	struct ehci_data data;
	unsigned long flags;
	uint32_t mode;
	int portno;
	enum usb_phy_interface phymode;
	struct param_d *param_mode;
	int role_registered;
	struct regulator *vbus;
	struct phy *phy;
	struct usb_phy *usbphy;
};

static int imx_chipidea_port_init(void *drvdata)
{
	struct imx_chipidea *ci = drvdata;
	int ret;

	if ((ci->flags & MXC_EHCI_PORTSC_MASK) == MXC_EHCI_MODE_ULPI) {
		dev_dbg(ci->dev, "using ULPI phy\n");
		if (IS_ENABLED(CONFIG_USB_ULPI)) {
			ret = ulpi_setup(ci->base + 0x170, 1);
			if (ret)
				dev_err(ci->dev, "ULPI setup failed with %s\n",
						strerror(-ret));
			mdelay(20);
		} else {
			dev_err(ci->dev, "no ULPI support available\n");
			ret = -ENODEV;
		}

		if (ret)
			return ret;
	}

	ret = imx_usbmisc_port_init(ci->portno, ci->flags);
	if (ret)
		dev_err(ci->dev, "misc init failed: %s\n", strerror(-ret));

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
	struct of_phandle_args out_args;
	enum usb_dr_mode mode;

	if (of_parse_phandle_with_args(ci->dev->device_node, "fsl,usbmisc",
					"#index-cells", 0, &out_args))
		return -ENODEV;

	ci->portno = out_args.args[0];
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
	case USB_DR_MODE_OTG:
		ci->mode = IMX_USB_MODE_OTG;
		break;
	case USB_DR_MODE_UNKNOWN:
		/*
		 * No dr_mode specified. This means it can either be OTG
		 * for port 0 or host mode for the other host-only ports.
		 */
		if (ci->portno == 0)
			ci->mode = IMX_USB_MODE_OTG;
		else
			ci->mode = IMX_USB_MODE_HOST;
		break;
	}

	ci->phymode = of_usb_get_phy_mode(ci->dev->device_node, NULL);
	switch (ci->phymode) {
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
		dev_dbg(ci->dev, "no phy_type setting. Relying on reset default\n");
	}

	if (of_find_property(ci->dev->device_node,
				"disable-over-current", NULL))
		ci->flags |= MXC_EHCI_DISABLE_OVERCURRENT;

	return 0;
}

static int ci_register_role(struct imx_chipidea *ci)
{
	int ret;

	if (ci->role_registered != IMX_USB_MODE_OTG)
		return -EBUSY;

	if (ci->mode == IMX_USB_MODE_HOST) {
		if (IS_ENABLED(CONFIG_USB_EHCI)) {
			ci->role_registered = 1;
			ret = regulator_enable(ci->vbus);
			if (ret)
				return ret;

			ret = ehci_register(ci->dev, &ci->data);
			if (!ret)
				return 0;

			regulator_disable(ci->vbus);
		} else {
			dev_err(ci->dev, "Host support not available\n");
			return -ENODEV;
		}
	}

	if (ci->mode == IMX_USB_MODE_DEVICE) {
		if (IS_ENABLED(CONFIG_USB_GADGET_DRIVER_ARC)) {
			ci->role_registered = 1;
			return ci_udc_register(ci->dev, ci->base);
		} else {
			dev_err(ci->dev, "USB device support not available\n");
			return -ENODEV;
		}
	}

	return 0;
}

static int ci_set_mode(struct param_d *param, void *priv)
{
	struct imx_chipidea *ci = priv;

	if (ci->role_registered != IMX_USB_MODE_OTG) {
		if (ci->role_registered == ci->mode)
			return 0;
		else
			return -EBUSY;
	}

	return ci_register_role(ci);
}

static const char *ci_mode_names[] = {
	"host", "peripheral", "otg"
};

static struct device_d imx_otg_device = {
	.name = "otg",
	.id = DEVICE_ID_SINGLE,
};

static int ci_register_otg_device(struct imx_chipidea *ci)
{
	int ret;

	if (imx_otg_device.parent)
		return -EBUSY;

	imx_otg_device.parent = ci->dev;

	ret = register_device(&imx_otg_device);
	if (ret)
		return ret;

	ci->param_mode = dev_add_param_enum(&imx_otg_device, "mode",
			ci_set_mode, NULL, &ci->mode,
			ci_mode_names, ARRAY_SIZE(ci_mode_names), ci);
	return 0;
}

static int imx_chipidea_probe(struct device_d *dev)
{
	struct resource *iores;
	struct imxusb_platformdata *pdata = dev->platform_data;
	int ret;
	void __iomem *base;
	struct imx_chipidea *ci;
	uint32_t portsc;

	ci = xzalloc(sizeof(*ci));
	ci->dev = dev;
	ci->role_registered = IMX_USB_MODE_OTG;

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
		ci->phymode = pdata->phymode;
		ci->mode = pdata->mode;
	}

	ci->vbus = regulator_get(dev, "vbus");
	if (IS_ERR(ci->vbus))
		ci->vbus = NULL;

	if (of_property_read_bool(dev->device_node, "fsl,usbphy")) {
		ci->phy = of_phy_get_by_phandle(dev, "fsl,usbphy", 0);
		if (IS_ERR(ci->phy)) {
			ret = PTR_ERR(ci->phy);
			dev_err(dev, "Cannot get phy: %s\n", strerror(-ret));
			return ret;
		} else {
			ci->usbphy = phy_to_usbphy(ci->phy);
			if (IS_ERR(ci->usbphy))
				return PTR_ERR(ci->usbphy);

			ret = phy_init(ci->phy);
			if (ret)
				return ret;
		}
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	ci->base = base;

	ci->data.init = imx_chipidea_port_init;
	ci->data.post_init = imx_chipidea_port_post_init;
	ci->data.drvdata = ci;
	ci->data.usbphy = ci->usbphy;

	if ((ci->flags & MXC_EHCI_PORTSC_MASK) == MXC_EHCI_MODE_HSIC)
		imx_chipidea_port_init(ci);

	if (ci->phymode != USBPHY_INTERFACE_MODE_UNKNOWN) {
		portsc = readl(base + 0x184);
		portsc &= ~MXC_EHCI_PORTSC_MASK;
		portsc |= ci->flags & MXC_EHCI_PORTSC_MASK;
		writel(portsc, base + 0x184);
	}

	ci->data.hccr = base + 0x100;
	ci->data.hcor = base + 0x140;
	ci->data.flags = EHCI_HAS_TT;

	if (ci->mode == IMX_USB_MODE_OTG)
		ret = ci_register_otg_device(ci);
	else
		ret = ci_register_role(ci);

	return ret;
};

static void imx_chipidea_remove(struct device_d *dev)
{
	if (IS_ENABLED(CONFIG_USB_GADGET_DRIVER_ARC))
		ci_udc_unregister();
}

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
	.remove = imx_chipidea_remove,
};
device_platform_driver(imx_chipidea_driver);

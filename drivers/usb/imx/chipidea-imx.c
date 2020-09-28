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
#include <linux/clk.h>

#define MXC_EHCI_PORTSC_MASK ((0xf << 28) | (1 << 25))

struct imx_chipidea_data {
	bool have_usb_misc;
};

struct imx_chipidea {
	struct device_d *dev;
	void __iomem *base;
	struct ehci_data data;
	unsigned long flags;
	enum usb_dr_mode mode;
	int portno;
	struct device_d *usbmisc;
	enum usb_phy_interface phymode;
	struct param_d *param_mode;
	struct regulator *vbus;
	struct phy *phy;
	struct usb_phy *usbphy;
	struct clk *clk;
	struct ehci_host *ehci;
	struct fsl_udc *udc;
	bool have_usb_misc;
};

static int imx_chipidea_port_init(void *drvdata)
{
	struct imx_chipidea *ci = drvdata;
	uint32_t portsc;
	int ret = 0;

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

	if (ci->have_usb_misc) {
		ret = imx_usbmisc_port_init(ci->usbmisc, ci->portno, ci->flags);
		if (ret)
			dev_err(ci->dev, "misc init failed: %s\n", strerror(-ret));
	}

	/* PFSC bit is reset by ehci_reset(), thus have to set it not in
	 * probe but here, after ehci_reset() is already called */
	if (ci->flags & MXC_EHCI_PFSC) {
		portsc = readl(ci->base + 0x184);
		portsc |= MXC_EHCI_PFSC;
		writel(portsc, ci->base + 0x184);
	}

	return ret;
}

static int imx_chipidea_port_post_init(void *drvdata)
{
	struct imx_chipidea *ci = drvdata;
	int ret = 0;

	if (ci->have_usb_misc) {
		ret = imx_usbmisc_port_post_init(ci->usbmisc, ci->portno, ci->flags);
		if (ret)
			dev_err(ci->dev, "post misc init failed: %s\n", strerror(-ret));
	}

	return ret;
}

static int imx_chipidea_probe_dt(struct imx_chipidea *ci)
{
	struct of_phandle_args out_args;

	if (ci->have_usb_misc) {
		if (of_parse_phandle_with_args(ci->dev->device_node, "fsl,usbmisc",
						"#index-cells", 0, &out_args))
			return -ENODEV;

		ci->usbmisc = of_find_device_by_node(out_args.np);
		if (!ci->usbmisc)
			return -ENODEV;

		ci->portno = out_args.args[0];
	}

	ci->flags = MXC_EHCI_MODE_UTMI_8BIT;

	ci->mode = of_usb_get_dr_mode(ci->dev->device_node, NULL);

	if (ci->mode == USB_DR_MODE_UNKNOWN) {
		/*
		 * No dr_mode specified. This means it can either be OTG
		 * for port 0 or host mode for the other host-only ports.
		 */
		if (ci->portno == 0)
			ci->mode = USB_DR_MODE_OTG;
		else
			ci->mode = USB_DR_MODE_HOST;
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

	else if (!of_find_property(ci->dev->device_node,
				   "over-current-active-high", NULL))
		ci->flags |= MXC_EHCI_OC_PIN_ACTIVE_LOW;

	if (of_usb_get_maximum_speed(ci->dev->device_node, NULL) ==
			USB_SPEED_FULL)
		ci->flags |= MXC_EHCI_PFSC;

	return 0;
}

static int ci_ehci_detect(struct device_d *dev)
{
	struct imx_chipidea *ci = dev->priv;

	return ehci_detect(ci->ehci);
}

static int ci_set_mode(void *ctx, enum usb_dr_mode mode)
{
	struct imx_chipidea *ci = ctx;
	int ret;

	if (mode == USB_DR_MODE_HOST) {
		if (IS_ENABLED(CONFIG_USB_EHCI)) {
			struct ehci_host *ehci;

			ret = regulator_enable(ci->vbus);
			if (ret)
				return ret;

			ehci = ehci_register(ci->dev, &ci->data);
			if (IS_ERR(ehci)) {
				regulator_disable(ci->vbus);
				return PTR_ERR(ehci);
			}

			ci->ehci = ehci;

			ci->dev->detect = ci_ehci_detect;
		} else {
			dev_err(ci->dev, "Host support not available\n");
			return -ENODEV;
		}
	}

	if (mode == USB_DR_MODE_PERIPHERAL) {
		if (IS_ENABLED(CONFIG_USB_GADGET_DRIVER_ARC)) {
			struct fsl_udc *udc;

			udc = ci_udc_register(ci->dev, ci->base);
			if (IS_ERR(udc))
				return PTR_ERR(udc);

			ci->udc = udc;
		} else {
			dev_err(ci->dev, "USB device support not available\n");
			return -ENODEV;
		}
	}

	return 0;
}

static int imx_chipidea_probe(struct device_d *dev)
{
	struct resource *iores;
	struct imx_chipidea_data *imx_data;
	struct imxusb_platformdata *pdata = dev->platform_data;
	int ret;
	void __iomem *base;
	struct imx_chipidea *ci;
	uint32_t portsc;

	ci = xzalloc(sizeof(*ci));
	ci->dev = dev;
	dev->priv = ci;

	ret = dev_get_drvdata(dev, (const void **)&imx_data);
	if (!ret)
		ci->have_usb_misc = imx_data->have_usb_misc;

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

	/*
	 * Some devices have more than one clock, in this case they are enabled
	 * by default in the clock driver. At least enable the main clock for
	 * devices which have only one.
	 */
	ci->clk = clk_get(dev, NULL);
	if (!IS_ERR(ci->clk))
		clk_enable(ci->clk);

	if (of_property_read_bool(dev->device_node, "fsl,usbphy")) {
		ci->phy = of_phy_get_by_phandle(dev, "fsl,usbphy", 0);
		if (IS_ERR(ci->phy)) {
			dev_err(dev, "Cannot get phy: %pe\n", ci->phy);
			return PTR_ERR(ci->phy);
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

	if (ci->mode == USB_DR_MODE_OTG)
		ret = usb_register_otg_device(ci->dev, ci_set_mode, ci);
	else
		ret = ci_set_mode(ci, ci->mode);

	return ret;
};

static void imx_chipidea_remove(struct device_d *dev)
{
	struct imx_chipidea *ci = dev->priv;

	if (ci->ehci)
		ehci_unregister(ci->ehci);

	if (IS_ENABLED(CONFIG_USB_GADGET_DRIVER_ARC) && ci->udc)
		ci_udc_unregister(ci->udc);
}

static const struct imx_chipidea_data imx_data = {
	.have_usb_misc = 1,
};

static const struct imx_chipidea_data imx28_data = {
	.have_usb_misc = 0,
};

static __maybe_unused struct of_device_id imx_chipidea_dt_ids[] = {
	{
		.compatible = "fsl,imx27-usb",
		.data = &imx_data,
	}, {
		.compatible = "fsl,imx28-usb",
		.data = &imx28_data,
	}, {
		.compatible = "fsl,imx7d-usb",
		.data = &imx_data,
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

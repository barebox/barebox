/*
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <malloc.h>
#include <usb/phy.h>
#include <linux/phy/phy.h>
#include <linux/clk.h>
#include <linux/err.h>

#define SET				0x4
#define CLR				0x8

#define USBPHY_CTRL			0x30

#define USBPHY_CTRL_SFTRST		(1 << 31)
#define USBPHY_CTRL_CLKGATE		(1 << 30)
#define USBPHY_CTRL_ENUTMILEVEL3	(1 << 15)
#define USBPHY_CTRL_ENUTMILEVEL2	(1 << 14)
#define USBPHY_CTRL_ENHOSTDISCONDETECT	(1 << 1)

struct imx_usbphy {
	struct usb_phy usb_phy;
	struct phy *phy;
	void __iomem *base;
	struct clk *clk;
	struct phy_provider *provider;
};

static int imx_usbphy_phy_init(struct phy *phy)
{
	struct imx_usbphy *imxphy = phy_get_drvdata(phy);

	clk_enable(imxphy->clk);

	/* reset usbphy */
	writel(USBPHY_CTRL_SFTRST, imxphy->base + USBPHY_CTRL + SET);

	udelay(10);

	/* clr reset and clkgate */
	writel(USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE,
			imxphy->base + USBPHY_CTRL + CLR);

	/* clr all pwd bits => power up phy */
	writel(0xffffffff, imxphy->base + CLR);

	/* set utmilvl2/3 */
	writel(USBPHY_CTRL_ENUTMILEVEL3 | USBPHY_CTRL_ENUTMILEVEL2,
	       imxphy->base + USBPHY_CTRL + SET);

	return 0;
}

static int imx_usbphy_notify_connect(struct usb_phy *phy,
				     enum usb_device_speed speed)
{
	struct imx_usbphy *imxphy = container_of(phy, struct imx_usbphy, usb_phy);

	if (speed == USB_SPEED_HIGH) {
		writel(USBPHY_CTRL_ENHOSTDISCONDETECT,
		       imxphy->base + USBPHY_CTRL + SET);
	}

	return 0;
}

static int imx_usbphy_notify_disconnect(struct usb_phy *phy,
					enum usb_device_speed speed)
{
	struct imx_usbphy *imxphy = container_of(phy, struct imx_usbphy, usb_phy);

	writel(USBPHY_CTRL_ENHOSTDISCONDETECT,
	       imxphy->base + USBPHY_CTRL + CLR);

	return 0;
}

static struct phy *imx_usbphy_xlate(struct device_d *dev,
				    struct of_phandle_args *args)
{
	struct imx_usbphy *imxphy = dev->priv;

	return imxphy->phy;
}

static struct usb_phy *imx_usbphy_to_usbphy(struct phy *phy)
{
	struct imx_usbphy *imxphy = phy_get_drvdata(phy);

	return &imxphy->usb_phy;
}

static const struct phy_ops imx_phy_ops = {
	.init = imx_usbphy_phy_init,
	.to_usbphy = imx_usbphy_to_usbphy,
};

static int imx_usbphy_probe(struct device_d *dev)
{
	struct resource *iores;
	int ret;
	struct imx_usbphy *imxphy;

	imxphy = xzalloc(sizeof(*imxphy));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	imxphy->base = IOMEM(iores->start);

	imxphy->clk = clk_get(dev, NULL);
	if (IS_ERR(imxphy->clk)) {
		dev_err(dev, "could not get clk: %s\n", strerrorp(imxphy->clk));
		ret = PTR_ERR(imxphy->clk);
		goto err_clk;
	}

	dev->priv = imxphy;

	imxphy->usb_phy.dev = dev;
	imxphy->usb_phy.notify_connect = imx_usbphy_notify_connect;
	imxphy->usb_phy.notify_disconnect = imx_usbphy_notify_disconnect;
	imxphy->phy = phy_create(dev, NULL, &imx_phy_ops, NULL);
	if (IS_ERR(imxphy->phy)) {
		ret = PTR_ERR(imxphy->phy);
		goto err_clk;
	}

	phy_set_drvdata(imxphy->phy, imxphy);

	imxphy->provider = of_phy_provider_register(dev, imx_usbphy_xlate);
	if (IS_ERR(imxphy->provider)) {
		ret = PTR_ERR(imxphy->provider);
		goto err_clk;
	}

	dev_dbg(dev, "phy enabled\n");

	return 0;

err_clk:
err_free:
	free(imxphy);

	return ret;
};

static __maybe_unused struct of_device_id imx_usbphy_dt_ids[] = {
	{
		.compatible = "fsl,imx23-usbphy",
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_usbphy_driver = {
	.name   = "imx-usb-phy",
	.probe  = imx_usbphy_probe,
	.of_compatible = DRV_OF_COMPAT(imx_usbphy_dt_ids),
};

static int imx_usbphy_init(void)
{
	return platform_driver_register(&imx_usbphy_driver);
}
fs_initcall(imx_usbphy_init);

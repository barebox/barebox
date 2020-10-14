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
#include <stmp-device.h>
#include <mfd/syscon.h>

#define HW_USBPHY_PWD				0x00
#define HW_USBPHY_TX				0x10
#define HW_USBPHY_CTRL				0x30
#define HW_USBPHY_CTRL_SET			0x34
#define HW_USBPHY_CTRL_CLR			0x38

#define BM_USBPHY_CTRL_ENUTMILEVEL3		BIT(15)
#define BM_USBPHY_CTRL_ENUTMILEVEL2		BIT(14)
#define BM_USBPHY_CTRL_ENHOSTDISCONDETECT	BIT(1)

#define ANADIG_USB1_CHRG_DETECT_SET		0x1b4
#define ANADIG_USB2_CHRG_DETECT_SET		0x214
#define ANADIG_USB1_CHRG_DETECT_EN_B		BIT(20)
#define ANADIG_USB1_CHRG_DETECT_CHK_CHRG_B	BIT(19)

struct imx_usbphy {
	struct usb_phy usb_phy;
	struct phy *phy;
	void __iomem *base;
	void __iomem *anatop;
	struct clk *clk;
	struct phy_provider *provider;
	int port_id;
};

static int imx_usbphy_phy_init(struct phy *phy)
{
	struct imx_usbphy *imxphy = phy_get_drvdata(phy);
	int ret;

	clk_enable(imxphy->clk);
	mdelay(1);

	ret = stmp_reset_block(imxphy->base + HW_USBPHY_CTRL, false);
	if (ret)
		return ret;

	/* Power up the PHY */
	writel(0, imxphy->base + HW_USBPHY_PWD);

	/* set utmilvl2/3 */
	writel(BM_USBPHY_CTRL_ENUTMILEVEL3 | BM_USBPHY_CTRL_ENUTMILEVEL2,
	       imxphy->base + HW_USBPHY_CTRL_SET);

	if (imxphy->anatop) {
		unsigned int reg = imxphy->port_id ?
			ANADIG_USB1_CHRG_DETECT_SET :
			ANADIG_USB2_CHRG_DETECT_SET;
		/*
		 * The external charger detector needs to be disabled,
		 * or the signal at DP will be poor
		 */
		writel(ANADIG_USB1_CHRG_DETECT_EN_B |
		       ANADIG_USB1_CHRG_DETECT_CHK_CHRG_B,
		       imxphy->anatop + reg);
	}

	return 0;
}

static int imx_usbphy_notify_connect(struct usb_phy *phy,
				     enum usb_device_speed speed)
{
	struct imx_usbphy *imxphy = container_of(phy, struct imx_usbphy,
						 usb_phy);

	if (speed == USB_SPEED_HIGH) {
		writel(BM_USBPHY_CTRL_ENHOSTDISCONDETECT,
		       imxphy->base + HW_USBPHY_CTRL_SET);
	}

	return 0;
}

static int imx_usbphy_notify_disconnect(struct usb_phy *phy,
					enum usb_device_speed speed)
{
	struct imx_usbphy *imxphy = container_of(phy, struct imx_usbphy,
						 usb_phy);

	writel(BM_USBPHY_CTRL_ENHOSTDISCONDETECT,
	       imxphy->base + HW_USBPHY_CTRL_CLR);

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
	struct device_node *np = dev->device_node;
	int ret;
	struct imx_usbphy *imxphy;

	imxphy = xzalloc(sizeof(*imxphy));

	ret = of_alias_get_id(np, "usbphy");
	if (ret < 0) {
		dev_dbg(dev, "failed to get alias id, errno %d\n", ret);
		goto err_free;
	}
	imxphy->port_id = ret;

	if (of_get_property(np, "fsl,anatop", NULL)) {
		imxphy->anatop =
			syscon_base_lookup_by_phandle(np, "fsl,anatop");
		ret = PTR_ERR_OR_ZERO(imxphy->anatop);
		if (ret)
			goto err_free;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	imxphy->base = IOMEM(iores->start);

	imxphy->clk = clk_get(dev, NULL);
	if (IS_ERR(imxphy->clk)) {
		dev_err(dev, "could not get clk: %pe\n", imxphy->clk);
		ret = PTR_ERR(imxphy->clk);
		goto err_clk;
	}

	dev->priv = imxphy;

	imxphy->usb_phy.dev = dev;
	imxphy->usb_phy.notify_connect = imx_usbphy_notify_connect;
	imxphy->usb_phy.notify_disconnect = imx_usbphy_notify_disconnect;
	imxphy->phy = phy_create(dev, NULL, &imx_phy_ops);
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
		.compatible = "fsl,vf610-usbphy",
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_usbphy_driver = {
	.name   = "imx-usb-phy",
	.probe  = imx_usbphy_probe,
	.of_compatible = DRV_OF_COMPAT(imx_usbphy_dt_ids),
};

fs_platform_driver(imx_usbphy_driver);

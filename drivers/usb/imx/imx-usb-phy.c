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
#include <driver.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

#define SET				0x4
#define CLR				0x8

#define USBPHY_CTRL			0x30

#define USBPHY_CTRL_SFTRST		(1 << 31)
#define USBPHY_CTRL_CLKGATE		(1 << 30)
#define USBPHY_CTRL_ENUTMILEVEL3	(1 << 15)
#define USBPHY_CTRL_ENUTMILEVEL2	(1 << 14)

struct imx_usbphy {
	void __iomem *base;
	struct clk *clk;
};

static int imx_usbphy_enable(struct imx_usbphy *imxphy)
{
	u32 val;

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
	val = readl(imxphy->base + USBPHY_CTRL);
	val |= USBPHY_CTRL_ENUTMILEVEL3 | USBPHY_CTRL_ENUTMILEVEL2;
	writel(val, imxphy->base + USBPHY_CTRL + SET);

	return 0;
}

static int imx_usbphy_probe(struct device_d *dev)
{
	int ret;
	struct imx_usbphy *imxphy;

	imxphy = xzalloc(sizeof(*imxphy));

	imxphy->base = dev_request_mem_region(dev, 0);
	if (!imxphy->base) {
		ret = -ENODEV;
		goto err_free;
	}

	imxphy->clk = clk_get(dev, NULL);
	if (IS_ERR(imxphy->clk)) {
		dev_err(dev, "could not get clk: %s\n", strerror(-PTR_ERR(imxphy->clk)));
		goto err_clk;
	}

	imx_usbphy_enable(imxphy);

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

// SPDX-License-Identifier: GPL-2.0
/*
 * dwc3-imx8mp.c - NXP imx8mp Specific Glue layer
 *
 * Copyright (c) 2020 NXP.
 */

#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <of.h>
#include <linux/device.h>

/* USB wakeup registers */
#define USB_WAKEUP_CTRL			0x00

/* Global wakeup interrupt enable, also used to clear interrupt */
#define USB_WAKEUP_EN			BIT(31)
/* Wakeup from connect or disconnect, only for superspeed */
#define USB_WAKEUP_SS_CONN		BIT(5)
/* 0 select vbus_valid, 1 select sessvld */
#define USB_WAKEUP_VBUS_SRC_SESS_VAL	BIT(4)
/* Enable signal for wake up from u3 state */
#define USB_WAKEUP_U3_EN		BIT(3)
/* Enable signal for wake up from id change */
#define USB_WAKEUP_ID_EN		BIT(2)
/* Enable signal for wake up from vbus change */
#define	USB_WAKEUP_VBUS_EN		BIT(1)
/* Enable signal for wake up from dp/dm change */
#define USB_WAKEUP_DPDM_EN		BIT(0)

#define USB_WAKEUP_EN_MASK		GENMASK(5, 0)

/* USB glue registers */
#define USB_CTRL0		0x00
#define USB_CTRL1		0x04

#define USB_CTRL0_PORTPWR_EN	BIT(12) /* 1 - PPC enabled (default) */
#define USB_CTRL0_USB3_FIXED	BIT(22) /* 1 - USB3 permanent attached */
#define USB_CTRL0_USB2_FIXED	BIT(23) /* 1 - USB2 permanent attached */

#define USB_CTRL1_OC_POLARITY	BIT(16) /* 0 - HIGH / 1 - LOW */
#define USB_CTRL1_PWR_POLARITY	BIT(17) /* 0 - HIGH / 1 - LOW */

struct dwc3_imx8mp {
	struct device			*dev;
	struct platform_device		*dwc3;
	void __iomem			*hsio_blk_base;
	void __iomem			*glue_base;
	struct clk			*hsio_clk;
	struct clk			*suspend_clk;
	bool				wakeup_pending;
};

static void imx8mp_configure_glue(struct dwc3_imx8mp *dwc3_imx)
{
	struct device *dev = dwc3_imx->dev;
	struct device_node *np = dev_of_node(dev);
	u32 value;

	if (!dwc3_imx->glue_base)
		return;

	value = readl(dwc3_imx->glue_base + USB_CTRL0);

	if (of_property_read_bool(np, "fsl,permanently-attached"))
		value |= (USB_CTRL0_USB2_FIXED | USB_CTRL0_USB3_FIXED);
	else
		value &= ~(USB_CTRL0_USB2_FIXED | USB_CTRL0_USB3_FIXED);

	if (of_property_read_bool(np, "fsl,disable-port-power-control"))
		value &= ~(USB_CTRL0_PORTPWR_EN);
	else
		value |= USB_CTRL0_PORTPWR_EN;

	writel(value, dwc3_imx->glue_base + USB_CTRL0);

	value = readl(dwc3_imx->glue_base + USB_CTRL1);
	if (of_property_read_bool(np, "fsl,over-current-active-low"))
		value |= USB_CTRL1_OC_POLARITY;
	else
		value &= ~USB_CTRL1_OC_POLARITY;

	if (of_property_read_bool(np, "fsl,power-active-low"))
		value |= USB_CTRL1_PWR_POLARITY;
	else
		value &= ~USB_CTRL1_PWR_POLARITY;

	writel(value, dwc3_imx->glue_base + USB_CTRL1);
}

static int dwc3_imx8mp_probe(struct device *dev)
{
	struct device_node	*node = dev->of_node;
	struct dwc3_imx8mp	*dwc3_imx;
	struct resource		*res;
	int			err;

	if (!node) {
		dev_err(dev, "device node not found\n");
		return -EINVAL;
	}

	dwc3_imx = devm_kzalloc(dev, sizeof(*dwc3_imx), GFP_KERNEL);
	if (!dwc3_imx)
		return -ENOMEM;

	dwc3_imx->dev = dev;

	dwc3_imx->hsio_blk_base = dev_platform_ioremap_resource(dev, 0);
	if (IS_ERR(dwc3_imx->hsio_blk_base))
		return PTR_ERR(dwc3_imx->hsio_blk_base);

	res = dev_request_mem_resource(dev, 1);
	if (!res)
		dev_warn(dev, "Base address for glue layer missing. Continuing without, some features are missing though.");
	else
		dwc3_imx->glue_base = IOMEM(res->start);

	dwc3_imx->hsio_clk = clk_get_enabled(dev, "hsio");
	if (IS_ERR(dwc3_imx->hsio_clk))
		return dev_err_probe(dev, PTR_ERR(dwc3_imx->hsio_clk),
				     "Failed to get hsio clk\n");

	dwc3_imx->suspend_clk = clk_get_enabled(dev, "suspend");
	if (IS_ERR(dwc3_imx->suspend_clk))
		return dev_err_probe(dev, PTR_ERR(dwc3_imx->suspend_clk),
				     "Failed to get suspend clk\n");

	imx8mp_configure_glue(dwc3_imx);

	err = of_platform_populate(node, NULL, dev);
	if (err) {
		dev_err(dev, "failed to create dwc3 core\n");
		return err;
	}

	return 0;
}

static const struct of_device_id dwc3_imx8mp_of_match[] = {
	{ .compatible = "fsl,imx8mp-dwc3", },
	{},
};
MODULE_DEVICE_TABLE(of, dwc3_imx8mp_of_match);

static struct driver dwc3_imx8mp_driver = {
	.probe		= dwc3_imx8mp_probe,
	.name	= "imx8mp-dwc3",
	.of_match_table	= dwc3_imx8mp_of_match,
};

device_platform_driver(dwc3_imx8mp_driver);

MODULE_ALIAS("platform:imx8mp-dwc3");
MODULE_AUTHOR("jun.li@nxp.com");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 imx8mp Glue Layer");

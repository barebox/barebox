// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2017 NXP. */

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


#define PHY_CTRL0			0x0
#define PHY_CTRL0_REF_SSP_EN		BIT(2)

#define PHY_CTRL1			0x4
#define PHY_CTRL1_RESET			BIT(0)
#define PHY_CTRL1_COMMONONN		BIT(1)
#define PHY_CTRL1_ATERESET		BIT(3)
#define PHY_CTRL1_VDATSRCENB0		BIT(19)
#define PHY_CTRL1_VDATDETENB0		BIT(20)

#define PHY_CTRL2			0x8
#define PHY_CTRL2_TXENABLEN0		BIT(8)

struct imx8mq_usb_phy {
	struct phy *phy;
	struct clk *clk;
	void __iomem *base;
};

static int imx8mq_usb_phy_init(struct phy *phy)
{
	struct imx8mq_usb_phy *imx_phy = phy_get_drvdata(phy);
	u32 value;

	value = readl(imx_phy->base + PHY_CTRL1);
	value &= ~(PHY_CTRL1_VDATSRCENB0 | PHY_CTRL1_VDATDETENB0 |
		   PHY_CTRL1_COMMONONN);
	value |= PHY_CTRL1_RESET | PHY_CTRL1_ATERESET;
	writel(value, imx_phy->base + PHY_CTRL1);

	value = readl(imx_phy->base + PHY_CTRL0);
	value |= PHY_CTRL0_REF_SSP_EN;
	writel(value, imx_phy->base + PHY_CTRL0);

	value = readl(imx_phy->base + PHY_CTRL2);
	value |= PHY_CTRL2_TXENABLEN0;
	writel(value, imx_phy->base + PHY_CTRL2);

	value = readl(imx_phy->base + PHY_CTRL1);
	value &= ~(PHY_CTRL1_RESET | PHY_CTRL1_ATERESET);
	writel(value, imx_phy->base + PHY_CTRL1);

	return 0;
}

static int imx8mq_phy_power_on(struct phy *phy)
{
	struct imx8mq_usb_phy *imx_phy = phy_get_drvdata(phy);

	return clk_enable(imx_phy->clk);
}

static int imx8mq_phy_power_off(struct phy *phy)
{
	struct imx8mq_usb_phy *imx_phy = phy_get_drvdata(phy);

	clk_disable(imx_phy->clk);

	return 0;
}

static struct phy_ops imx8mq_usb_phy_ops = {
	.init		= imx8mq_usb_phy_init,
	.power_on	= imx8mq_phy_power_on,
	.power_off	= imx8mq_phy_power_off,
};

static struct phy *imx8mq_usb_phy_xlate(struct device_d *dev,
					struct of_phandle_args *args)
{
	struct imx8mq_usb_phy *imx_phy = dev->priv;

	return imx_phy->phy;
}

static int imx8mq_usb_phy_probe(struct device_d *dev)
{
	struct phy_provider *phy_provider;
	struct imx8mq_usb_phy *imx_phy;

	imx_phy = xzalloc(sizeof(*imx_phy));

	dev->priv = imx_phy;

	imx_phy->clk = clk_get(dev, "phy");
	if (IS_ERR(imx_phy->clk))
		return PTR_ERR(imx_phy->clk);

	imx_phy->base = dev_get_mem_region(dev, 0);
	if (IS_ERR(imx_phy->base))
		return PTR_ERR(imx_phy->base);

	imx_phy->phy = phy_create(dev, NULL, &imx8mq_usb_phy_ops, NULL);
	if (IS_ERR(imx_phy->phy))
		return PTR_ERR(imx_phy->phy);

	phy_set_drvdata(imx_phy->phy, imx_phy);

	phy_provider = of_phy_provider_register(dev, imx8mq_usb_phy_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id imx8mq_usb_phy_of_match[] = {
	{.compatible = "fsl,imx8mq-usb-phy",},
	{ },
};

static struct driver_d imx8mq_usb_phy_driver = {
	.name	= "imx8mq-usb-phy",
	.probe	= imx8mq_usb_phy_probe,
	.of_compatible = DRV_OF_COMPAT(imx8mq_usb_phy_of_match),
};
device_platform_driver(imx8mq_usb_phy_driver);

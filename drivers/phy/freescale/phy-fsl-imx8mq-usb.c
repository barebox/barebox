// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) 2017 NXP. */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/phy/phy.h>
#include <malloc.h>
#include <of_device.h>
#include <of.h>
#include <linux/usb/phy.h>


#define PHY_CTRL0			0x0
#define PHY_CTRL0_REF_SSP_EN		BIT(2)
#define PHY_CTRL0_FSEL_MASK		GENMASK(10, 5)
#define PHY_CTRL0_FSEL_24M		0x2a

#define PHY_CTRL1			0x4
#define PHY_CTRL1_RESET			BIT(0)
#define PHY_CTRL1_COMMONONN		BIT(1)
#define PHY_CTRL1_ATERESET		BIT(3)
#define PHY_CTRL1_VDATSRCENB0		BIT(19)
#define PHY_CTRL1_VDATDETENB0		BIT(20)

#define PHY_CTRL2			0x8
#define PHY_CTRL2_TXENABLEN0		BIT(8)
#define PHY_CTRL2_OTG_DISABLE		BIT(9)

#define PHY_CTRL6			0x18
#define PHY_CTRL6_ALT_CLK_EN		BIT(1)
#define PHY_CTRL6_ALT_CLK_SEL		BIT(0)

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

static int imx8mp_usb_phy_init(struct phy *phy)
{
	struct imx8mq_usb_phy *imx_phy = phy_get_drvdata(phy);
	u32 value;

	/* USB3.0 PHY signal fsel for 24M ref */
	value = readl(imx_phy->base + PHY_CTRL0);
	value &= ~PHY_CTRL0_FSEL_MASK;
	value |= FIELD_PREP(PHY_CTRL0_FSEL_MASK, PHY_CTRL0_FSEL_24M);
	writel(value, imx_phy->base + PHY_CTRL0);

	/* Disable alt_clk_en and use internal MPLL clocks */
	value = readl(imx_phy->base + PHY_CTRL6);
	value &= ~(PHY_CTRL6_ALT_CLK_SEL | PHY_CTRL6_ALT_CLK_EN);
	writel(value, imx_phy->base + PHY_CTRL6);

	value = readl(imx_phy->base + PHY_CTRL1);
	value &= ~(PHY_CTRL1_VDATSRCENB0 | PHY_CTRL1_VDATDETENB0);
	value |= PHY_CTRL1_RESET | PHY_CTRL1_ATERESET;
	writel(value, imx_phy->base + PHY_CTRL1);

	value = readl(imx_phy->base + PHY_CTRL0);
	value |= PHY_CTRL0_REF_SSP_EN;
	writel(value, imx_phy->base + PHY_CTRL0);

	value = readl(imx_phy->base + PHY_CTRL2);
	value |= PHY_CTRL2_TXENABLEN0 | PHY_CTRL2_OTG_DISABLE;
	writel(value, imx_phy->base + PHY_CTRL2);

	udelay(10);

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

static const struct phy_ops imx8mq_usb_phy_ops = {
	.init		= imx8mq_usb_phy_init,
	.power_on	= imx8mq_phy_power_on,
	.power_off	= imx8mq_phy_power_off,
};

static const struct phy_ops imx8mp_usb_phy_ops = {
	.init		= imx8mp_usb_phy_init,
	.power_on	= imx8mq_phy_power_on,
	.power_off	= imx8mq_phy_power_off,
};

static const struct of_device_id imx8mq_usb_phy_of_match[] = {
	{.compatible = "fsl,imx8mq-usb-phy",
	 .data = &imx8mq_usb_phy_ops,},
	{.compatible = "fsl,imx8mp-usb-phy",
	 .data = &imx8mp_usb_phy_ops,},
	{ }
};
MODULE_DEVICE_TABLE(of, imx8mq_usb_phy_of_match);

static struct phy *imx8mq_usb_phy_xlate(struct device *dev,
					struct of_phandle_args *args)
{
	struct imx8mq_usb_phy *imx_phy = dev->priv;

	return imx_phy->phy;
}

static int imx8mq_usb_phy_probe(struct device *dev)
{
	struct phy_provider *phy_provider;
	struct imx8mq_usb_phy *imx_phy;
	const struct phy_ops *phy_ops;

	imx_phy = xzalloc(sizeof(*imx_phy));

	dev->priv = imx_phy;

	imx_phy->clk = clk_get(dev, "phy");
	if (IS_ERR(imx_phy->clk))
		return PTR_ERR(imx_phy->clk);

	imx_phy->base = dev_get_mem_region(dev, 0);
	if (IS_ERR(imx_phy->base))
		return PTR_ERR(imx_phy->base);

	phy_ops = of_device_get_match_data(dev);
	if (!phy_ops)
		return -EINVAL;

	imx_phy->phy = phy_create(dev, NULL, phy_ops);
	if (IS_ERR(imx_phy->phy))
		return PTR_ERR(imx_phy->phy);

	phy_set_drvdata(imx_phy->phy, imx_phy);

	phy_provider = of_phy_provider_register(dev, imx8mq_usb_phy_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct driver imx8mq_usb_phy_driver = {
	.name	= "imx8mq-usb-phy",
	.probe	= imx8mq_usb_phy_probe,
	.of_compatible = DRV_OF_COMPAT(imx8mq_usb_phy_of_match),
};
device_platform_driver(imx8mq_usb_phy_driver);

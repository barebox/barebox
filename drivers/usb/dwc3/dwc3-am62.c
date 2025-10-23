// SPDX-License-Identifier: GPL-2.0
/*
 * dwc3-am62.c - TI specific Glue layer for AM62 DWC3 USB Controller
 *
 * Copyright (C) 2022 Texas Instruments Incorporated - https://www.ti.com
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/units.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/pinctrl/consumer.h>
#include <linux/device.h>
#include <mfd/syscon.h>

#include "core.h"

/* USB WRAPPER register offsets */
#define USBSS_PID			0x0
#define USBSS_OVERCURRENT_CTRL		0x4
#define USBSS_PHY_CONFIG		0x8
#define USBSS_PHY_TEST			0xc
#define USBSS_CORE_STAT			0x14
#define USBSS_HOST_VBUS_CTRL		0x18
#define USBSS_MODE_CONTROL		0x1c
#define USBSS_WAKEUP_CONFIG		0x30
#define USBSS_WAKEUP_STAT		0x34
#define USBSS_OVERRIDE_CONFIG		0x38
#define USBSS_IRQ_MISC_STATUS_RAW	0x430
#define USBSS_IRQ_MISC_STATUS		0x434
#define USBSS_IRQ_MISC_ENABLE_SET	0x438
#define USBSS_IRQ_MISC_ENABLE_CLR	0x43c
#define USBSS_IRQ_MISC_EOI		0x440
#define USBSS_INTR_TEST			0x490
#define USBSS_VBUS_FILTER		0x614
#define USBSS_VBUS_STAT			0x618
#define USBSS_DEBUG_CFG			0x708
#define USBSS_DEBUG_DATA		0x70c
#define USBSS_HOST_HUB_CTRL		0x714

/* PHY CONFIG register bits */
#define USBSS_PHY_VBUS_SEL_MASK		GENMASK(2, 1)
#define USBSS_PHY_VBUS_SEL_SHIFT	1
#define USBSS_PHY_LANE_REVERSE		BIT(0)

/* CORE STAT register bits */
#define USBSS_CORE_OPERATIONAL_MODE_MASK	GENMASK(13, 12)
#define USBSS_CORE_OPERATIONAL_MODE_SHIFT	12

/* MODE CONTROL register bits */
#define USBSS_MODE_VALID	BIT(0)

/* WAKEUP CONFIG register bits */
#define USBSS_WAKEUP_CFG_OVERCURRENT_EN	BIT(3)
#define USBSS_WAKEUP_CFG_LINESTATE_EN	BIT(2)
#define USBSS_WAKEUP_CFG_SESSVALID_EN	BIT(1)
#define USBSS_WAKEUP_CFG_VBUSVALID_EN	BIT(0)

#define USBSS_WAKEUP_CFG_ALL	(USBSS_WAKEUP_CFG_VBUSVALID_EN | \
				 USBSS_WAKEUP_CFG_SESSVALID_EN | \
				 USBSS_WAKEUP_CFG_LINESTATE_EN | \
				 USBSS_WAKEUP_CFG_OVERCURRENT_EN)

#define USBSS_WAKEUP_CFG_NONE	0

/* WAKEUP STAT register bits */
#define USBSS_WAKEUP_STAT_OVERCURRENT	BIT(4)
#define USBSS_WAKEUP_STAT_LINESTATE	BIT(3)
#define USBSS_WAKEUP_STAT_SESSVALID	BIT(2)
#define USBSS_WAKEUP_STAT_VBUSVALID	BIT(1)
#define USBSS_WAKEUP_STAT_CLR		BIT(0)

/* IRQ_MISC_STATUS_RAW register bits */
#define USBSS_IRQ_MISC_RAW_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_RAW_SESSVALID	BIT(20)

/* IRQ_MISC_STATUS register bits */
#define USBSS_IRQ_MISC_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_SESSVALID	BIT(20)

/* IRQ_MISC_ENABLE_SET register bits */
#define USBSS_IRQ_MISC_ENABLE_SET_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_ENABLE_SET_SESSVALID	BIT(20)

/* IRQ_MISC_ENABLE_CLR register bits */
#define USBSS_IRQ_MISC_ENABLE_CLR_VBUSVALID	BIT(22)
#define USBSS_IRQ_MISC_ENABLE_CLR_SESSVALID	BIT(20)

/* IRQ_MISC_EOI register bits */
#define USBSS_IRQ_MISC_EOI_VECTOR	BIT(0)

/* VBUS_STAT register bits */
#define USBSS_VBUS_STAT_SESSVALID	BIT(2)
#define USBSS_VBUS_STAT_VBUSVALID	BIT(0)

/* USB_PHY_CTRL register bits in CTRL_MMR */
#define PHY_CORE_VOLTAGE_MASK	BIT(31)
#define PHY_PLL_REFCLK_MASK	GENMASK(3, 0)

/* USB PHY2 register offsets */
#define	USB_PHY_PLL_REG12		0x130
#define	USB_PHY_PLL_LDO_REF_EN		BIT(5)
#define	USB_PHY_PLL_LDO_REF_EN_EN	BIT(4)

#define DWC3_AM62_AUTOSUSPEND_DELAY	100

struct dwc3_am62 {
	struct device *dev;
	void __iomem *usbss;
	struct clk *usb2_refclk;
	int rate_code;
	struct regmap *syscon;
	unsigned int offset;
	unsigned int vbus_divider;
	u32 wakeup_stat;
};

static const int dwc3_ti_rate_table[] = {	/* in KHZ */
	9600,
	10000,
	12000,
	19200,
	20000,
	24000,
	25000,
	26000,
	38400,
	40000,
	58000,
	50000,
	52000,
};

static inline u32 dwc3_ti_readl(struct dwc3_am62 *am62, u32 offset)
{
	return readl((am62->usbss) + offset);
}

static inline void dwc3_ti_writel(struct dwc3_am62 *am62, u32 offset, u32 value)
{
	writel(value, (am62->usbss) + offset);
}

static int phy_syscon_pll_refclk(struct dwc3_am62 *am62)
{
	struct device *dev = am62->dev;
	struct device_node *node = dev->of_node;
	struct regmap *syscon;
	int ret;

	syscon = syscon_regmap_lookup_by_phandle(node, "ti,syscon-phy-pll-refclk");
	if (IS_ERR(syscon)) {
		dev_err(dev, "unable to get ti,syscon-phy-pll-refclk regmap\n");
		return PTR_ERR(syscon);
	}

	am62->syscon = syscon;

	ret = of_property_read_u32_index(node, "ti,syscon-phy-pll-refclk", 1, &am62->offset);
	if (ret)
		return ret;

	/* Core voltage. PHY_CORE_VOLTAGE bit Recommended to be 0 always */
	ret = regmap_update_bits(am62->syscon, am62->offset, PHY_CORE_VOLTAGE_MASK, 0);
	if (ret) {
		dev_err(dev, "failed to set phy core voltage\n");
		return ret;
	}

	ret = regmap_update_bits(am62->syscon, am62->offset, PHY_PLL_REFCLK_MASK, am62->rate_code);
	if (ret) {
		dev_err(dev, "failed to set phy pll reference clock rate\n");
		return ret;
	}

	return 0;
}

static int dwc3_ti_probe(struct device *dev)
{
	struct device_node *node = dev->of_node;
	struct dwc3_am62 *am62;
	unsigned long rate;
	void __iomem *phy;
	int i, ret;
	u32 reg;

	am62 = devm_kzalloc(dev, sizeof(*am62), GFP_KERNEL);
	if (!am62)
		return -ENOMEM;

	am62->dev = dev;
	dev->priv = am62;

	am62->usbss = dev_request_mem_region(dev, 0);
	if (IS_ERR(am62->usbss))
		return dev_err_probe(dev, PTR_ERR(am62->usbss), "can't map IOMEM resource\n");

	am62->usb2_refclk = clk_get(dev, "ref");
	if (IS_ERR(am62->usb2_refclk))
		return dev_err_probe(dev, PTR_ERR(am62->usb2_refclk), "can't get usb2_refclk\n");

	/* Calculate the rate code */
	rate = clk_get_rate(am62->usb2_refclk);
	rate /= HZ_PER_KHZ;
	for (i = 0; i < ARRAY_SIZE(dwc3_ti_rate_table); i++) {
		if (dwc3_ti_rate_table[i] == rate)
			break;
	}

	if (i == ARRAY_SIZE(dwc3_ti_rate_table))
		return dev_err_probe(dev, -EINVAL, "unsupported usb2_refclk rate: %lu KHz\n", rate);

	am62->rate_code = i;

	/* Read the syscon property and set the rate code */
	ret = phy_syscon_pll_refclk(am62);
	if (ret)
		return ret;

	/* Workaround Errata i2409 */
	phy = dev_request_mem_region(dev, 1);
	if (IS_ERR(phy)) {
		dev_err(dev, "can't map PHY IOMEM resource. Won't apply i2409 fix.\n");
		phy = NULL;
	} else {
		reg = readl(phy + USB_PHY_PLL_REG12);
		reg |= USB_PHY_PLL_LDO_REF_EN | USB_PHY_PLL_LDO_REF_EN_EN;
		writel(reg, phy + USB_PHY_PLL_REG12);
	}

	/* VBUS divider select */
	am62->vbus_divider = of_property_read_bool(node, "ti,vbus-divider");
	reg = dwc3_ti_readl(am62, USBSS_PHY_CONFIG);
	if (am62->vbus_divider)
		reg |= 1 << USBSS_PHY_VBUS_SEL_SHIFT;

	dwc3_ti_writel(am62, USBSS_PHY_CONFIG, reg);

	clk_prepare_enable(am62->usb2_refclk);

	/* Set mode valid bit to indicate role is valid */
	reg = dwc3_ti_readl(am62, USBSS_MODE_CONTROL);
	reg |= USBSS_MODE_VALID;
	dwc3_ti_writel(am62, USBSS_MODE_CONTROL, reg);

	ret = of_platform_populate(node, NULL, dev);
	if (ret) {
		dev_err_probe(dev, ret, "failed to create dwc3 core\n");
		goto err_pm_disable;
	}

	return 0;

err_pm_disable:
	clk_disable_unprepare(am62->usb2_refclk);

	return ret;
}

static const struct of_device_id dwc3_ti_of_match[] = {
	{ .compatible = "ti,am62-usb"},
	{},
};
MODULE_DEVICE_TABLE(of, dwc3_ti_of_match);

static struct driver dwc3_ti_driver = {
	.probe = dwc3_ti_probe,
	.name = "dwc3-of-simple",
	.of_compatible = DRV_OF_COMPAT(dwc3_ti_of_match),
};
device_platform_driver(dwc3_ti_driver);

MODULE_ALIAS("platform:dwc3-am62");
MODULE_AUTHOR("Aswath Govindraju <a-govindraju@ti.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DesignWare USB3 TI Glue Layer");

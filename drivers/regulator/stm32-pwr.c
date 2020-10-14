// SPDX-License-Identifier: GPL-2.0
// Copyright (C) STMicroelectronics 2019
// Authors: Gabriel Fernandez <gabriel.fernandez@st.com>
//          Pascal Paillet <p.paillet@st.com>.

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/iopoll.h>
#include <of.h>
#include <regulator.h>

/*
 * Registers description
 */
#define REG_PWR_CR3 0x0C

#define USB_3_3_EN BIT(24)
#define USB_3_3_RDY BIT(26)
#define REG_1_8_EN BIT(28)
#define REG_1_8_RDY BIT(29)
#define REG_1_1_EN BIT(30)
#define REG_1_1_RDY BIT(31)

/* list of supported regulators */
enum {
	PWR_REG11,
	PWR_REG18,
	PWR_USB33,
	STM32PWR_REG_NUM_REGS
};

struct stm32_pwr_desc {
	struct regulator_desc desc;
	const char *name;
	const char *supply_name;
};

static u32 ready_mask_table[STM32PWR_REG_NUM_REGS] = {
	[PWR_REG11] = REG_1_1_RDY,
	[PWR_REG18] = REG_1_8_RDY,
	[PWR_USB33] = USB_3_3_RDY,
};

struct stm32_pwr_reg {
	void __iomem *base;
	u32 ready_mask;
	struct regulator_dev rdev;
	struct regulator *supply;
};

static inline struct stm32_pwr_reg *to_pwr_reg(struct regulator_dev *rdev)
{
	return container_of(rdev, struct stm32_pwr_reg, rdev);
}

static inline struct stm32_pwr_desc *to_desc(struct regulator_dev *rdev)
{
	return container_of(rdev->desc, struct stm32_pwr_desc, desc);
}

static int stm32_pwr_reg_is_ready(struct regulator_dev *rdev)
{
	struct stm32_pwr_reg *priv = to_pwr_reg(rdev);
	u32 val;

	val = readl(priv->base + REG_PWR_CR3);

	return (val & priv->ready_mask);
}

static int stm32_pwr_reg_is_enabled(struct regulator_dev *rdev)
{
	struct stm32_pwr_reg *priv = to_pwr_reg(rdev);
	u32 val;

	val = readl(priv->base + REG_PWR_CR3);

	return (val & rdev->desc->enable_mask);
}

static int stm32_pwr_reg_enable(struct regulator_dev *rdev)
{
	struct stm32_pwr_reg *priv = to_pwr_reg(rdev);
	struct stm32_pwr_desc *desc = to_desc(rdev);
	int ret;
	u32 val;

	regulator_enable(priv->supply);

	val = readl(priv->base + REG_PWR_CR3);
	val |= rdev->desc->enable_mask;
	writel(val, priv->base + REG_PWR_CR3);

	/* use an arbitrary timeout of 20ms */
	ret = readx_poll_timeout(stm32_pwr_reg_is_ready, rdev, val, val,
				 20 * USEC_PER_MSEC);
	if (ret)
		dev_err(rdev->dev, "%s: regulator enable timed out!\n",
			desc->name);

	return ret;
}

static int stm32_pwr_reg_disable(struct regulator_dev *rdev)
{
	struct stm32_pwr_reg *priv = to_pwr_reg(rdev);
	struct stm32_pwr_desc *desc = to_desc(rdev);
	int ret;
	u32 val;

	val = readl(priv->base + REG_PWR_CR3);
	val &= ~rdev->desc->enable_mask;
	writel(val, priv->base + REG_PWR_CR3);

	/* use an arbitrary timeout of 20ms */
	ret = readx_poll_timeout(stm32_pwr_reg_is_ready, rdev, val, !val,
				 20 * USEC_PER_MSEC);
	if (ret)
		dev_err(rdev->dev, "%s: regulator disable timed out!\n",
			desc->name);

	regulator_disable(priv->supply);

	return ret;
}

static const struct regulator_ops stm32_pwr_reg_ops = {
	.enable		= stm32_pwr_reg_enable,
	.disable	= stm32_pwr_reg_disable,
	.is_enabled	= stm32_pwr_reg_is_enabled,
};

#define PWR_REG(_id, _name, _volt, _en, _supply) \
	[_id] = { { \
		.n_voltages = 1, \
		.ops = &stm32_pwr_reg_ops, \
		.min_uV = _volt, \
		.enable_mask = _en, \
	}, .name = _name, .supply_name = _supply, }

static const struct stm32_pwr_desc stm32_pwr_desc[] = {
	PWR_REG(PWR_REG11, "reg11", 1100000, REG_1_1_EN, "vdd"),
	PWR_REG(PWR_REG18, "reg18", 1800000, REG_1_8_EN, "vdd"),
	PWR_REG(PWR_USB33, "usb33", 3300000, USB_3_3_EN, "vdd_3v3_usbfs"),
};

static int stm32_pwr_regulator_probe(struct device_d *dev)
{
	struct stm32_pwr_reg *priv;
	struct device_node *child;
	struct resource *iores;
	int i, ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	for_each_child_of_node(dev->device_node, child) {
		const struct stm32_pwr_desc *desc = NULL;

		for (i = 0; i < STM32PWR_REG_NUM_REGS; i++) {
			const char *name;

			name = of_get_property(child, "regulator-name", NULL);
			if (name && strcmp(stm32_pwr_desc[i].name, name)) {
				desc = &stm32_pwr_desc[i];
				break;
			}
		}

		if (!desc) {
			dev_warn(dev, "Skipping unknown child node %s\n",
				 child->name);
			continue;
		}

		priv = xzalloc(sizeof(*priv));
		priv->base = IOMEM(iores->start);
		priv->ready_mask = ready_mask_table[i];

		priv->rdev.desc = &desc->desc;
		priv->rdev.dev = dev;

		priv->supply = regulator_get(dev, desc->supply_name);
		if (IS_ERR(priv->supply))
			return PTR_ERR(priv->supply);

		ret = of_regulator_register(&priv->rdev, child);
		if (ret) {
			dev_err(dev, "%s: Failed to register regulator: %d\n",
				desc->name, ret);
			return ret;
		}
	}

	return 0;
}

static const struct of_device_id stm32_pwr_of_match[] = {
	{ .compatible = "st,stm32mp1,pwr-reg", },
	{ /* sentinel */ },
};

static struct driver_d stm32_pwr_driver = {
	.probe = stm32_pwr_regulator_probe,
	.name  = "stm32-pwr-regulator",
	.of_compatible = stm32_pwr_of_match,
};
device_platform_driver(stm32_pwr_driver);

MODULE_DESCRIPTION("STM32MP1 PWR voltage regulator driver");
MODULE_AUTHOR("Pascal Paillet <p.paillet@st.com>");
MODULE_LICENSE("GPL v2");

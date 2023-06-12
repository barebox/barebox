// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) STMicroelectronics 2017
 *
 * Author: Fabrice Gasnier <fabrice.gasnier@st.com>
 */

#include <common.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <of.h>
#include <io.h>
#include <regulator.h>

/* STM32 VREFBUF registers */
#define STM32_VREFBUF_CSR		0x00

/* STM32 VREFBUF CSR bitfields */
#define STM32_VRS			GENMASK(6, 4)
#define STM32_VRR			BIT(3)
#define STM32_HIZ			BIT(1)
#define STM32_ENVR			BIT(0)

#define STM32_VREFBUF_AUTO_SUSPEND_DELAY_MS	10

struct stm32_vrefbuf {
	void __iomem *base;
	struct clk *clk;
	struct device *dev;
	struct regulator_dev rdev;
};

struct stm32_vrefbuf_desc {
	struct regulator_desc desc;
	const char *supply_name;
};

static inline struct stm32_vrefbuf *to_stm32_vrefbuf(struct regulator_dev *rdev)
{
	return container_of(rdev, struct stm32_vrefbuf, rdev);
}

static const unsigned int stm32_vrefbuf_voltages[] = {
	/* Matches resp. VRS = 000b, 001b, 010b, 011b */
	2500000, 2048000, 1800000, 1500000,
};

static int stm32_vrefbuf_enable(struct regulator_dev *rdev)
{
	struct stm32_vrefbuf *priv = to_stm32_vrefbuf(rdev);
	u32 val;
	int ret;

	val = readl_relaxed(priv->base + STM32_VREFBUF_CSR);
	val = (val & ~STM32_HIZ) | STM32_ENVR;
	writel_relaxed(val, priv->base + STM32_VREFBUF_CSR);

	/*
	 * Vrefbuf startup time depends on external capacitor: wait here for
	 * VRR to be set. That means output has reached expected value.
	 * ~650us sleep should be enough for caps up to 1.5uF. Use 10ms as
	 * arbitrary timeout.
	 */
	ret = readl_poll_timeout(priv->base + STM32_VREFBUF_CSR, val,
				 val & STM32_VRR, 10000);
	if (ret) {
		dev_err(priv->dev, "stm32 vrefbuf timed out!\n");
		val = readl_relaxed(priv->base + STM32_VREFBUF_CSR);
		val = (val & ~STM32_ENVR) | STM32_HIZ;
		writel_relaxed(val, priv->base + STM32_VREFBUF_CSR);
	}

	return ret;
}

static int stm32_vrefbuf_disable(struct regulator_dev *rdev)
{
	struct stm32_vrefbuf *priv = to_stm32_vrefbuf(rdev);
	u32 val;

	val = readl_relaxed(priv->base + STM32_VREFBUF_CSR);
	val &= ~STM32_ENVR;
	writel_relaxed(val, priv->base + STM32_VREFBUF_CSR);

	return 0;
}

static int stm32_vrefbuf_is_enabled(struct regulator_dev *rdev)
{
	struct stm32_vrefbuf *priv = to_stm32_vrefbuf(rdev);
	int ret;

	ret = readl_relaxed(priv->base + STM32_VREFBUF_CSR) & STM32_ENVR;

	return ret;
}

static int stm32_vrefbuf_set_voltage_sel(struct regulator_dev *rdev,
					 unsigned sel)
{
	struct stm32_vrefbuf *priv = to_stm32_vrefbuf(rdev);
	u32 val;

	val = readl_relaxed(priv->base + STM32_VREFBUF_CSR);
	val = (val & ~STM32_VRS) | FIELD_PREP(STM32_VRS, sel);
	writel_relaxed(val, priv->base + STM32_VREFBUF_CSR);

	return 0;
}

static int stm32_vrefbuf_get_voltage_sel(struct regulator_dev *rdev)
{
	struct stm32_vrefbuf *priv = to_stm32_vrefbuf(rdev);
	u32 val;
	int ret;

	val = readl_relaxed(priv->base + STM32_VREFBUF_CSR);
	ret = FIELD_GET(STM32_VRS, val);

	return ret;
}

static const struct regulator_ops stm32_vrefbuf_volt_ops = {
	.enable		= stm32_vrefbuf_enable,
	.disable	= stm32_vrefbuf_disable,
	.is_enabled	= stm32_vrefbuf_is_enabled,
	.get_voltage_sel = stm32_vrefbuf_get_voltage_sel,
	.set_voltage_sel = stm32_vrefbuf_set_voltage_sel,
	.list_voltage	= regulator_list_voltage_table,
};

static const struct stm32_vrefbuf_desc stm32_vrefbuf_regu = {
	.desc = {
		.volt_table = stm32_vrefbuf_voltages,
		.n_voltages = ARRAY_SIZE(stm32_vrefbuf_voltages),
		.ops = &stm32_vrefbuf_volt_ops,
		.off_on_delay = 1000,
	},
	.supply_name = "vdda",
};

static int stm32_vrefbuf_probe(struct device *dev)
{
	struct stm32_vrefbuf *priv;
	struct regulator_dev *rdev;
	struct regulator *supply;
	int ret;

	supply = regulator_get(dev, stm32_vrefbuf_regu.supply_name);
	if (IS_ERR(supply))
		return PTR_ERR(supply);

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;

	priv->base = dev_request_mem_region(dev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	ret = clk_enable(priv->clk);
	if (ret) {
		dev_err(dev, "clk enable failed with error %d\n", ret);
		return ret;
	}

	rdev = &priv->rdev;

	rdev->dev = dev;
	rdev->desc = &stm32_vrefbuf_regu.desc;

	ret = of_regulator_register(rdev, dev->of_node);
	if (ret) {
		ret = PTR_ERR(rdev);
		dev_err(dev, "register failed with error %d\n", ret);
		goto err_clk_dis;
	}

	regulator_enable(supply);

	dev->priv = priv;

	return 0;

err_clk_dis:
	clk_disable(priv->clk);

	return ret;
}

static void stm32_vrefbuf_remove(struct device *dev)
{
	struct stm32_vrefbuf *priv = dev->priv;

	clk_disable(priv->clk);
};

static const struct of_device_id __maybe_unused stm32_vrefbuf_of_match[] = {
	{ .compatible = "st,stm32-vrefbuf", },
	{},
};
MODULE_DEVICE_TABLE(of, stm32_vrefbuf_of_match);

static struct driver stm32_vrefbuf_driver = {
	.probe = stm32_vrefbuf_probe,
	.name  = "stm32-vrefbuf",
	.remove = stm32_vrefbuf_remove,
	.of_compatible = stm32_vrefbuf_of_match,
};
device_platform_driver(stm32_vrefbuf_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Fabrice Gasnier <fabrice.gasnier@st.com>");
MODULE_DESCRIPTION("STMicroelectronics STM32 VREFBUF driver");
MODULE_ALIAS("platform:stm32-vrefbuf");

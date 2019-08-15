// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017, STMicroelectronics - All Rights Reserved
 * Copyright (C) 2019, Ahmad Fatoum, Pengutronix
 * Author(s): Patrice Chotard, <patrice.chotard@st.com> for STMicroelectronics.
 */

#include <common.h>
#include <init.h>
#include <linux/err.h>
#include <linux/reset-controller.h>
#include <asm/io.h>

#define RCC_CL 0x4

struct stm32_reset {
	void __iomem *base;
	struct reset_controller_dev rcdev;
	void (*reset)(void __iomem *reg, unsigned offset, bool assert);
};

static struct stm32_reset *to_stm32_reset(struct reset_controller_dev *rcdev)
{
	return container_of(rcdev, struct stm32_reset, rcdev);
}

static void stm32mp_reset(void __iomem *reg, unsigned offset, bool assert)
{
	if (!assert)
		reg += RCC_CL;

	writel(BIT(offset), reg);
}

static void stm32mcu_reset(void __iomem *reg, unsigned offset, bool assert)
{
	if (assert)
		setbits_le32(reg, BIT(offset));
	else
		clrbits_le32(reg, BIT(offset));
}

static void stm32_reset(struct stm32_reset *priv, unsigned long id, bool assert)
{
	int bank = (id / BITS_PER_LONG) * 4;
	int offset = id % BITS_PER_LONG;

	priv->reset(priv->base + bank, offset, assert);
}

static int stm32_reset_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	stm32_reset(to_stm32_reset(rcdev), id, true);
	return 0;
}

static int stm32_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	stm32_reset(to_stm32_reset(rcdev), id, false);
	return 0;
}

static const struct reset_control_ops stm32_reset_ops = {
	.assert		= stm32_reset_assert,
	.deassert	= stm32_reset_deassert,
};

static int stm32_reset_probe(struct device_d *dev)
{
	struct stm32_reset *priv;
	struct resource *iores;
	int ret;

	priv = xzalloc(sizeof(*priv));
	ret = dev_get_drvdata(dev, (const void **)&priv->reset);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv->base = IOMEM(iores->start);
	priv->rcdev.nr_resets = (iores->end - iores->start) * BITS_PER_BYTE;
	priv->rcdev.ops = &stm32_reset_ops;
	priv->rcdev.of_node = dev->device_node;

	return reset_controller_register(&priv->rcdev);
}

static const struct of_device_id stm32_rcc_reset_dt_ids[] = {
	{ .compatible = "st,stm32mp1-rcc", .data = stm32mp_reset },
	{ .compatible = "st,stm32-rcc", .data = stm32mcu_reset },
	{ /* sentinel */ },
};

static struct driver_d stm32_rcc_reset_driver = {
	.name = "stm32_rcc_reset",
	.probe = stm32_reset_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_rcc_reset_dt_ids),
};

static int stm32_rcc_reset_init(void)
{
	return platform_driver_register(&stm32_rcc_reset_driver);
}
postcore_initcall(stm32_rcc_reset_init);

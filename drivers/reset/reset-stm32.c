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
#include <restart.h>
#include <reset_source.h>
#include <asm/io.h>

#define RCC_CL 0x4

#define RCC_MP_GRSTCSETR		0x404
#define RCC_MP_RSTSCLRR			0x408

#define STM32MP_RCC_RSTF_POR		BIT(0)
#define STM32MP_RCC_RSTF_BOR		BIT(1)
#define STM32MP_RCC_RSTF_PAD		BIT(2)
#define STM32MP_RCC_RSTF_HCSS		BIT(3)
#define STM32MP_RCC_RSTF_VCORE		BIT(4)

#define STM32MP_RCC_RSTF_MPSYS		BIT(6)
#define STM32MP_RCC_RSTF_MCSYS		BIT(7)
#define STM32MP_RCC_RSTF_IWDG1		BIT(8)
#define STM32MP_RCC_RSTF_IWDG2		BIT(9)

#define STM32MP_RCC_RSTF_STDBY		BIT(11)
#define STM32MP_RCC_RSTF_CSTDBY		BIT(12)
#define STM32MP_RCC_RSTF_MPUP0		BIT(13)
#define STM32MP_RCC_RSTF_MPUP1		BIT(14)

struct stm32_reset_reason {
	uint32_t mask;
	enum reset_src_type type;
	int instance;
};

struct stm32_reset {
	void __iomem *base;
	struct reset_controller_dev rcdev;
	struct restart_handler restart;
	const struct stm32_reset_ops *ops;
};

struct stm32_reset_ops {
	void (*reset)(void __iomem *reg, unsigned offset, bool assert);
	void __noreturn (*sys_reset)(struct restart_handler *rst);
	const struct stm32_reset_reason *reset_reasons;
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

static u32 stm32_reset_status(struct stm32_reset *priv, unsigned long bank)
{
	return readl(priv->base + bank);
}

static void stm32_reset(struct stm32_reset *priv, unsigned long id, bool assert)
{
	int bank = (id / BITS_PER_LONG) * 4;
	int offset = id % BITS_PER_LONG;

	priv->ops->reset(priv->base + bank, offset, assert);
}

static void stm32_set_reset_reason(struct stm32_reset *priv,
				   const struct stm32_reset_reason *reasons)
{
	enum reset_src_type type = RESET_UKWN;
	u32 reg;
	int i, instance = 0;

	reg = stm32_reset_status(priv, RCC_MP_RSTSCLRR);

	for (i = 0; reasons[i].mask; i++) {
		if (reg & reasons[i].mask) {
			type     = reasons[i].type;
			instance = reasons[i].instance;
			break;
		}
	}

	reset_source_set_prinst(type, RESET_SOURCE_DEFAULT_PRIORITY, instance);

	pr_info("STM32 RCC reset reason %s (MP_RSTSR: 0x%08x)\n",
		reset_source_to_string(type), reg);
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
	ret = dev_get_drvdata(dev, (const void **)&priv->ops);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv->base = IOMEM(iores->start);
	priv->rcdev.nr_resets = (iores->end - iores->start) * BITS_PER_BYTE;
	priv->rcdev.ops = &stm32_reset_ops;
	priv->rcdev.of_node = dev->device_node;

	if (priv->ops->sys_reset) {
		priv->restart.name = "stm32-rcc";
		priv->restart.restart = priv->ops->sys_reset;
		priv->restart.priority = 200;

		ret = restart_handler_register(&priv->restart);
		if (ret)
			dev_warn(dev, "Cannot register restart handler\n");
	}

	if (priv->ops->reset_reasons)
		stm32_set_reset_reason(priv, priv->ops->reset_reasons);

	return reset_controller_register(&priv->rcdev);
}

static void __noreturn stm32mp_rcc_restart_handler(struct restart_handler *rst)
{
	struct stm32_reset *priv = container_of(rst, struct stm32_reset, restart);

	stm32_reset(priv, RCC_MP_GRSTCSETR * BITS_PER_BYTE, true);

	mdelay(1000);
	hang();
}

static const struct stm32_reset_reason stm32mp_reset_reasons[] = {
	{ STM32MP_RCC_RSTF_POR,		RESET_POR, 0 },
	{ STM32MP_RCC_RSTF_BOR,		RESET_BROWNOUT, 0 },
	{ STM32MP_RCC_RSTF_STDBY,	RESET_WKE, 0 },
	{ STM32MP_RCC_RSTF_CSTDBY,	RESET_WKE, 1 },
	{ STM32MP_RCC_RSTF_MPSYS,	RESET_RST, 2 },
	{ STM32MP_RCC_RSTF_MPUP0,	RESET_RST, 0 },
	{ STM32MP_RCC_RSTF_MPUP1,	RESET_RST, 1 },
	{ STM32MP_RCC_RSTF_IWDG1,	RESET_WDG, 0 },
	{ STM32MP_RCC_RSTF_IWDG2,	RESET_WDG, 1 },
	{ STM32MP_RCC_RSTF_PAD,		RESET_EXT, 1 },
	{ /* sentinel */ }
};

static const struct stm32_reset_ops stm32mp1_reset_ops = {
	.reset = stm32mp_reset,
	.sys_reset = stm32mp_rcc_restart_handler,
	.reset_reasons = stm32mp_reset_reasons,
};

static const struct stm32_reset_ops stm32mcu_reset_ops = {
	.reset = stm32mcu_reset,
};

static const struct of_device_id stm32_rcc_reset_dt_ids[] = {
	{ .compatible = "st,stm32mp1-rcc", .data = &stm32mp1_reset_ops },
	{ .compatible = "st,stm32-rcc", .data = &stm32mcu_reset_ops },
	{ /* sentinel */ },
};

static struct driver_d stm32_rcc_reset_driver = {
	.name = "stm32_rcc_reset",
	.probe = stm32_reset_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_rcc_reset_dt_ids),
};

postcore_platform_driver(stm32_rcc_reset_driver);

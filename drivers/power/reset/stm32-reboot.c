// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2017, STMicroelectronics - All Rights Reserved
 * Copyright (C) 2019, Ahmad Fatoum, Pengutronix
 * Author(s): Patrice Chotard, <patrice.chotard@st.com> for STMicroelectronics.
 */

#include <common.h>
#include <linux/err.h>
#include <restart.h>
#include <reset_source.h>
#include <of_address.h>
#include <asm/io.h>
#include <soc/stm32/reboot.h>

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
	struct restart_handler restart;
};

static u32 stm32_reset_status(struct stm32_reset *priv, unsigned long bank)
{
	return readl(priv->base + bank);
}

static void stm32_reset(struct stm32_reset *priv, unsigned long id, bool assert)
{
	int bank = (id / 32) * 4;
	int offset = id % 32;
	void __iomem *reg = priv->base + bank;

	if (!assert)
		reg += RCC_CL;

	writel(BIT(offset), reg);
}

static void __noreturn stm32mp_rcc_restart_handler(struct restart_handler *rst,
						   unsigned long flags)
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

void stm32mp_system_restart_init(struct device *dev)
{
	struct stm32_reset *priv;
	struct device_node *np = dev_of_node(dev);

	priv = xzalloc(sizeof(*priv));

	priv->base = of_iomap(np, 0);

	priv->restart.name = "stm32-rcc";
	priv->restart.restart = stm32mp_rcc_restart_handler;
	priv->restart.priority = 200;
	priv->restart.of_node = np;

	restart_handler_register(&priv->restart);

	stm32_set_reset_reason(priv, stm32mp_reset_reasons);
}

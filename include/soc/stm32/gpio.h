// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015 Maxime Coquelin
 * Copyright (C) 2017 STMicroelectronics
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#ifndef __STM32_GPIO_H__
#define __STM32_GPIO_H__

#include <io.h>

#define STM32_GPIO_MODER	0x00
#define STM32_GPIO_TYPER	0x04
#define STM32_GPIO_SPEEDR	0x08
#define STM32_GPIO_PUPDR	0x0c
#define STM32_GPIO_IDR		0x10
#define STM32_GPIO_ODR		0x14
#define STM32_GPIO_BSRR		0x18
#define STM32_GPIO_LCKR		0x1c
#define STM32_GPIO_AFRL		0x20
#define STM32_GPIO_AFRH		0x24

#define STM32_PIN_GPIO		0
#define STM32_PIN_AF(x)		((x) + 1)
#define STM32_PIN_ANALOG	(STM32_PIN_AF(15) + 1)

#define STM32_PINMODE_GPIO	0
#define STM32_PINMODE_AF	2
#define STM32_PINMODE_ANALOG	3

#define STM32_GPIO_PINS_PER_BANK	16

enum stm32_pin_bias { STM32_PIN_NO_BIAS, STM32_PIN_PULL_UP, STM32_PIN_PULL_DOWN };
enum stm32_pin_out_type { STM32_PIN_OUT_PUSHPULL, STM32_PIN_OUT_OPENDRAIN };

static inline void __stm32_pmx_set_speed(void __iomem  *base,
					 unsigned offset, u32 speed)
{
	u32 val = readl(base + STM32_GPIO_SPEEDR);
	val &= ~GENMASK(offset * 2 + 1, offset * 2);
	val |= speed << (offset * 2);
	writel(val, base + STM32_GPIO_SPEEDR);
}

static inline void __stm32_pmx_set_bias(void __iomem *base, unsigned offset,
					enum stm32_pin_bias bias)
{
	u32 val = readl(base + STM32_GPIO_PUPDR);
	val &= ~GENMASK(offset * 2 + 1, offset * 2);
	val |= bias << (offset * 2);
	writel(val, base + STM32_GPIO_PUPDR);
}

static inline void __stm32_pmx_set_mode(void __iomem *base,
					int pin, u32 mode, u32 alt)
{
	u32 val;
	int alt_shift = (pin % 8) * 4;
	int alt_offset = STM32_GPIO_AFRL + (pin / 8) * 4;

	val = readl(base + alt_offset);
	val &= ~GENMASK(alt_shift + 3, alt_shift);
	val |= (alt << alt_shift);
	writel(val, base + alt_offset);

	val = readl(base + STM32_GPIO_MODER);
	val &= ~GENMASK(pin * 2 + 1, pin * 2);
	val |= mode << (pin * 2);
	writel(val, base + STM32_GPIO_MODER);
}

static inline void __stm32_pmx_get_mode(void __iomem *base, int pin,
					u32 *mode, u32 *alt)
{
	u32 val;
	int alt_shift = (pin % 8) * 4;
	int alt_offset = STM32_GPIO_AFRL + (pin / 8) * 4;

	val = readl(base + alt_offset);
	val &= GENMASK(alt_shift + 3, alt_shift);
	*alt = val >> alt_shift;

	val = readl(base + STM32_GPIO_MODER);
	val &= GENMASK(pin * 2 + 1, pin * 2);
	*mode = val >> (pin * 2);
}

static inline int __stm32_pmx_gpio_get(void __iomem *base, unsigned offset)
{
	return !!(readl(base + STM32_GPIO_IDR) & BIT(offset));
}

static inline void __stm32_pmx_gpio_set(void __iomem *base, unsigned offset,
					unsigned value)
{
	if (!value)
		offset += STM32_GPIO_PINS_PER_BANK;

	writel(BIT(offset), base + STM32_GPIO_BSRR);
}

static inline void __stm32_pmx_gpio_input(void __iomem *base, unsigned offset)
{
	__stm32_pmx_set_mode(base, offset, 0, 0);
}

static inline void __stm32_pmx_gpio_output(void __iomem *base, unsigned offset,
					   unsigned value)
{
	__stm32_pmx_gpio_set(base, offset, value);
	__stm32_pmx_set_mode(base, offset, 1, 0);
}

static inline void __stm32_pmx_set_output_type(void __iomem *base, unsigned offset,
					       enum stm32_pin_out_type type)
{
	u32 val = readl(base + STM32_GPIO_TYPER);
	val &= ~BIT(offset);
	val |= type << offset;
	writel(val, base + STM32_GPIO_TYPER);
}

#endif /* __STM32_GPIO_H__ */

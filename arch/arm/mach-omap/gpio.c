/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * This work is derived from the linux 2.6.27 kernel source
 * To fetch, use the kernel repository
 * git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux-2.6.git
 * Use the v2.6.27 tag.
 *
 * Below is the original's header including its copyright
 *
 *  linux/arch/arm/plat-omap/gpio.c
 *
 * Support functions for OMAP GPIO
 *
 * Copyright (C) 2003-2005 Nokia Corporation
 * Written by Juha Yrjölä <juha.yrjola@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <errno.h>

#ifdef CONFIG_ARCH_OMAP3

#define OMAP_GPIO_OE		0x0034
#define OMAP_GPIO_DATAIN	0x0038
#define OMAP_GPIO_DATAOUT	0x003c
#define OMAP_GPIO_CLEARDATAOUT	0x0090
#define OMAP_GPIO_SETDATAOUT	0x0094

static void __iomem *gpio_bank[] = {
	(void *)0x48310000,
	(void *)0x49050000,
	(void *)0x49052000,
	(void *)0x49054000,
	(void *)0x49056000,
	(void *)0x49058000,
};
#endif

#ifdef CONFIG_ARCH_OMAP4

#define OMAP_GPIO_OE		0x0134
#define OMAP_GPIO_DATAIN	0x0138
#define OMAP_GPIO_DATAOUT	0x013c
#define OMAP_GPIO_CLEARDATAOUT	0x0190
#define OMAP_GPIO_SETDATAOUT	0x0194

static void __iomem *gpio_bank[] = {
	(void *)0x4a310000,
	(void *)0x48055000,
	(void *)0x48057000,
	(void *)0x48059000,
	(void *)0x4805b000,
	(void *)0x4805d000,
};
#endif

static inline void __iomem *get_gpio_base(int gpio)
{
	return gpio_bank[gpio >> 5];
}

static inline int get_gpio_index(int gpio)
{
	return gpio & 0x1f;
}

static inline int gpio_valid(int gpio)
{
	if (gpio < 0)
		return -1;
	if (gpio / 32 < ARRAY_SIZE(gpio_bank))
		return 0;
	return -1;
}

static int check_gpio(int gpio)
{
	if (gpio_valid(gpio) < 0) {
		printf("ERROR : check_gpio: invalid GPIO %d\n", gpio);
		return -1;
	}
	return 0;
}

void gpio_set_value(unsigned gpio, int value)
{
	void __iomem *reg;
	u32 l = 0;

	if (check_gpio(gpio) < 0)
		return;

	reg = get_gpio_base(gpio);

	if (value)
		reg += OMAP_GPIO_SETDATAOUT;
	else
		reg += OMAP_GPIO_CLEARDATAOUT;
	l = 1 << get_gpio_index(gpio);

	__raw_writel(l, reg);
}

int gpio_direction_input(unsigned gpio)
{
	void __iomem *reg;
	u32 val;

	if (check_gpio(gpio) < 0)
		return -EINVAL;

	reg = get_gpio_base(gpio);

	reg += OMAP_GPIO_OE;

	val = __raw_readl(reg);
	val |= 1 << get_gpio_index(gpio);
	__raw_writel(val, reg);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	void __iomem *reg;
	u32 val;

	if (check_gpio(gpio) < 0)
		return -EINVAL;
	reg = get_gpio_base(gpio);

	gpio_set_value(gpio, value);

	reg += OMAP_GPIO_OE;

	val = __raw_readl(reg);
	val &= ~(1 << get_gpio_index(gpio));
	__raw_writel(val, reg);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	void __iomem *reg;

	if (check_gpio(gpio) < 0)
		return -EINVAL;
	reg = get_gpio_base(gpio);

	reg += OMAP_GPIO_DATAIN;

	return (__raw_readl(reg) & (1 << get_gpio_index(gpio))) != 0;
}

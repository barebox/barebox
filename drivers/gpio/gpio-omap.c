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
#include <io.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>

#define OMAP_GPIO_OE		0x0034
#define OMAP_GPIO_DATAIN	0x0038
#define OMAP_GPIO_DATAOUT	0x003c
#define OMAP_GPIO_CLEARDATAOUT	0x0090
#define OMAP_GPIO_SETDATAOUT	0x0094

struct omap_gpio_chip {
	void __iomem *base;
	struct gpio_chip chip;
};

static inline int omap_get_gpio_index(int gpio)
{
	return gpio & 0x1f;
}

static void omap_gpio_set_value(struct gpio_chip *chip,
					unsigned gpio, int value)
{
	struct omap_gpio_chip *omapgpio =
			container_of(chip, struct omap_gpio_chip, chip);
	void __iomem *base = omapgpio->base;
	u32 l = 0;

	if (value)
		base += OMAP_GPIO_SETDATAOUT;
	else
		base += OMAP_GPIO_CLEARDATAOUT;

	l = 1 << omap_get_gpio_index(gpio);

	writel(l, base);
}

static int omap_gpio_direction_input(struct gpio_chip *chip,
					unsigned gpio)
{
	struct omap_gpio_chip *omapgpio =
			container_of(chip, struct omap_gpio_chip, chip);
	void __iomem *base = omapgpio->base;
	u32 val;

	base += OMAP_GPIO_OE;

	val = readl(base);
	val |= 1 << omap_get_gpio_index(gpio);
	writel(val, base);

	return 0;
}

static int omap_gpio_direction_output(struct gpio_chip *chip,
					unsigned gpio, int value)
{
	struct omap_gpio_chip *omapgpio =
			container_of(chip, struct omap_gpio_chip, chip);
	void __iomem *base = omapgpio->base;
	u32 val;

	omap_gpio_set_value(chip, gpio, value);

	base += OMAP_GPIO_OE;

	val = readl(base);
	val &= ~(1 << omap_get_gpio_index(gpio));
	writel(val, base);

	return 0;
}

static int omap_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct omap_gpio_chip *omapgpio =
			container_of(chip, struct omap_gpio_chip, chip);
	void __iomem *base = omapgpio->base;

	base  += OMAP_GPIO_DATAIN;

	return (readl(base) & (1 << omap_get_gpio_index(gpio))) != 0;

}

static struct gpio_ops omap_gpio_ops = {
	.direction_input = omap_gpio_direction_input,
	.direction_output = omap_gpio_direction_output,
	.get = omap_gpio_get_value,
	.set = omap_gpio_set_value,
};

static int omap_gpio_probe(struct device_d *dev)
{
	struct omap_gpio_chip *omapgpio;

	omapgpio = xzalloc(sizeof(*omapgpio));
	omapgpio->base = dev_request_mem_region(dev, 0);
	omapgpio->chip.ops = &omap_gpio_ops;
	omapgpio->chip.base = dev->id * 32;
	omapgpio->chip.ngpio = 32;
	omapgpio->chip.dev = dev;
	gpiochip_add(&omapgpio->chip);

	dev_dbg(dev, "probed gpiochip%d with base %d\n",
				dev->id, omapgpio->chip.base);

	return 0;
}

static struct driver_d omap_gpio_driver = {
	.name = "omap-gpio",
	.probe = omap_gpio_probe,
};

static int omap_gpio_add(void)
{
	return platform_driver_register(&omap_gpio_driver);
}
coredevice_initcall(omap_gpio_add);

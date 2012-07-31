/*
 *  arch/arm/mach-imx/gpio.c
 *
 *  author: Sascha Hauer
 *  Created: april 20th, 2004
 *  Copyright: Synertronixx GmbH
 *
 *  Common code for i.MX machines
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <mach/imx-regs.h>
#include <gpio.h>
#include <init.h>

#if defined CONFIG_ARCH_IMX1 || defined CONFIG_ARCH_IMX21 || defined CONFIG_ARCH_IMX27
#define GPIO_DR		0x1c
#define GPIO_GDIR	0x00
#define GPIO_PSR	0x24
#define GPIO_ICR1	0x28
#define GPIO_ICR2	0x2C
#define GPIO_IMR	0x30
#define GPIO_ISR	0x34
#else
#define GPIO_DR		0x00
#define GPIO_GDIR	0x04
#define GPIO_PSR	0x08
#define GPIO_ICR1	0x0C
#define GPIO_ICR2	0x10
#define GPIO_IMR	0x14
#define GPIO_ISR	0x18
#define GPIO_ISR	0x18
#endif

struct imx_gpio_chip {
	void __iomem *base;
	struct gpio_chip chip;
};

static void imx_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	if (!base)
		return;

	val = readl(base + GPIO_DR);

	if (value)
		val |= 1 << gpio;
	else
		val &= ~(1 << gpio);

	writel(val, base + GPIO_DR);
}

static int imx_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	if (!base)
		return -EINVAL;

	val = readl(base + GPIO_GDIR);
	val &= ~(1 << gpio);
	writel(val, base + GPIO_GDIR);

	return 0;
}


static int imx_gpio_direction_output(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	gpio_set_value(gpio + chip->base, value);

	val = readl(base + GPIO_GDIR);
	val |= 1 << gpio;
	writel(val, base + GPIO_GDIR);

	return 0;
}

static int imx_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	val = readl(base + GPIO_PSR);

	return val & (1 << gpio) ? 1 : 0;
}

static struct gpio_ops imx_gpio_ops = {
	.direction_input = imx_gpio_direction_input,
	.direction_output = imx_gpio_direction_output,
	.get = imx_gpio_get_value,
	.set = imx_gpio_set_value,
};

static int imx_gpio_probe(struct device_d *dev)
{
	struct imx_gpio_chip *imxgpio;

	imxgpio = xzalloc(sizeof(*imxgpio));
	imxgpio->base = dev_request_mem_region(dev, 0);
	imxgpio->chip.ops = &imx_gpio_ops;
	imxgpio->chip.base = -1;
	imxgpio->chip.ngpio = 32;
	imxgpio->chip.dev = dev;
	gpiochip_add(&imxgpio->chip);

	dev_info(dev, "probed gpiochip%d with base %d\n", dev->id, imxgpio->chip.base);

	return 0;
}

static struct driver_d imx_gpio_driver = {
	.name = "imx-gpio",
	.probe = imx_gpio_probe,
};

static int imx_gpio_add(void)
{
	register_driver(&imx_gpio_driver);
	return 0;
}
coredevice_initcall(imx_gpio_add);

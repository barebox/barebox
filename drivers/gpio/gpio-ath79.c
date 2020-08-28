/*
 *  Atheros AR71XX/AR724X/AR913X GPIO API support
 *
 *  Copyright (C) 2010-2011 Jaiganesh Narayanan <jnarayanan@atheros.com>
 *  Copyright (C) 2008-2011 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *  Copyright (C) 2015 Antony Pavlov <antonynpavlov@gmail.com>
 *
 *  Parts of this file are based on Atheros' 2.6.15/2.6.31 BSP
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <gpio.h>
#include <malloc.h>

#include <mach/ar71xx_regs.h>
#include <mach/ath79.h>

static void __iomem *ath79_gpio_base;
static u32 ath79_gpio_count;

static void __ath79_gpio_set_value(unsigned gpio, int value)
{
	void __iomem *base = ath79_gpio_base;

	if (value)
		__raw_writel(1 << gpio, base + AR71XX_GPIO_REG_SET);
	else
		__raw_writel(1 << gpio, base + AR71XX_GPIO_REG_CLEAR);
}

static int __ath79_gpio_get_value(unsigned gpio)
{
	return (__raw_readl(ath79_gpio_base + AR71XX_GPIO_REG_IN) >> gpio) & 1;
}

static int ath79_gpio_get_value(struct gpio_chip *chip, unsigned offset)
{
	return __ath79_gpio_get_value(offset);
}

static void ath79_gpio_set_value(struct gpio_chip *chip,
				  unsigned offset, int value)
{
	__ath79_gpio_set_value(offset, value);
}

static int ath79_gpio_direction_input(struct gpio_chip *chip,
				       unsigned offset)
{
	void __iomem *base = ath79_gpio_base;

	__raw_writel(__raw_readl(base + AR71XX_GPIO_REG_OE) & ~(1 << offset),
		     base + AR71XX_GPIO_REG_OE);

	return 0;
}

static int ath79_gpio_direction_output(struct gpio_chip *chip,
					unsigned offset, int value)
{
	void __iomem *base = ath79_gpio_base;

	if (value)
		__raw_writel(1 << offset, base + AR71XX_GPIO_REG_SET);
	else
		__raw_writel(1 << offset, base + AR71XX_GPIO_REG_CLEAR);

	__raw_writel(__raw_readl(base + AR71XX_GPIO_REG_OE) | (1 << offset),
		     base + AR71XX_GPIO_REG_OE);

	return 0;
}

static int ath79_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{

	void __iomem *base = ath79_gpio_base;
	uint32_t oe = __raw_readl(base + AR71XX_GPIO_REG_OE);

	return (oe & (1 << offset)) ? GPIOF_DIR_OUT : GPIOF_DIR_IN;
}

static struct gpio_ops ath79_gpio_ops = {
	.get			= ath79_gpio_get_value,
	.set			= ath79_gpio_set_value,
	.direction_input	= ath79_gpio_direction_input,
	.direction_output	= ath79_gpio_direction_output,
	.get_direction		= ath79_gpio_get_direction,
};

static struct gpio_chip ath79_gpio_chip = {
	.ops = &ath79_gpio_ops,
	.base = 0,
};

static const struct of_device_id ath79_gpio_of_match[] = {
	{ .compatible = "qca,ar7100-gpio" },
	{},
};

static int ath79_gpio_probe(struct device_d *dev)
{
	struct resource *iores;
	struct device_node *np = dev->device_node;
	int err;

	if (!np) {
		dev_err(dev, "No DT node or platform data found\n");
		return -EINVAL;
	}

	err = of_property_read_u32(np, "ngpios", &ath79_gpio_count);
	if (err) {
		dev_err(dev, "ngpios property is not valid\n");
		return err;
	}
	if (ath79_gpio_count >= 32) {
		dev_err(dev, "ngpios must be less than 32\n");
		return -EINVAL;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	ath79_gpio_base = IOMEM(iores->start);

	ath79_gpio_chip.dev = dev;
	ath79_gpio_chip.ngpio = ath79_gpio_count;

	err = gpiochip_add(&ath79_gpio_chip);
	if (err) {
		dev_err(dev, "cannot add AR71xx GPIO chip, error=%d", err);
		return err;
	}

	return 0;
}

static struct driver_d ath79_gpio_driver = {
	.name = "ath79-gpio",
	.probe = ath79_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(ath79_gpio_of_match),
};

coredevice_platform_driver(ath79_gpio_driver);

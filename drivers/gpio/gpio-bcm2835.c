/*
 * Author: Carlo Caione <carlo@carlocaione.org>
 *
 * Based on linux/arch/arm/mach-bcm2708/bcm2708_gpio.c
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
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <io.h>
#include <gpio.h>
#include <init.h>

#define GPIOFSEL(x)  (0x00+(x)*4)
#define GPIOSET(x)   (0x1c+(x)*4)
#define GPIOCLR(x)   (0x28+(x)*4)
#define GPIOLEV(x)   (0x34+(x)*4)
#define GPIOEDS(x)   (0x40+(x)*4)
#define GPIOREN(x)   (0x4c+(x)*4)
#define GPIOFEN(x)   (0x58+(x)*4)
#define GPIOHEN(x)   (0x64+(x)*4)
#define GPIOLEN(x)   (0x70+(x)*4)
#define GPIOAREN(x)  (0x7c+(x)*4)
#define GPIOAFEN(x)  (0x88+(x)*4)
#define GPIOUD(x)    (0x94+(x)*4)
#define GPIOUDCLK(x) (0x98+(x)*4)

enum {
	GPIO_FSEL_INPUT, GPIO_FSEL_OUTPUT,
	GPIO_FSEL_ALT5, GPIO_FSEL_ALT_4,
	GPIO_FSEL_ALT0, GPIO_FSEL_ALT1,
	GPIO_FSEL_ALT2, GPIO_FSEL_ALT3,
};

struct bcm2835_gpio_chip {
	void __iomem *base;
	struct gpio_chip chip;
};

static int bcm2835_set_function(struct gpio_chip *chip, unsigned gpio, int function)
{
	struct bcm2835_gpio_chip *bcmgpio = container_of(chip, struct bcm2835_gpio_chip, chip);
	void __iomem *base = bcmgpio->base;
	unsigned gpiodir;
	unsigned gpio_bank = gpio / 10;
	unsigned gpio_field_offset = (gpio - 10 * gpio_bank) * 3;

	gpiodir = readl(base + GPIOFSEL(gpio_bank));
	gpiodir &= ~(7 << gpio_field_offset);
	gpiodir |= function << gpio_field_offset;
	writel(gpiodir, base + GPIOFSEL(gpio_bank));
	gpiodir = readl(base + GPIOFSEL(gpio_bank));

	return 0;
}

static void bcm2835_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct bcm2835_gpio_chip *bcmgpio = container_of(chip, struct bcm2835_gpio_chip, chip);
	void __iomem *base = bcmgpio->base;
	unsigned gpio_bank = gpio / 32;
	unsigned gpio_field_offset = gpio % 32;

	if (value)
		writel(1 << gpio_field_offset, base + GPIOSET(gpio_bank));
	else
		writel(1 << gpio_field_offset, base + GPIOCLR(gpio_bank));
}

static int bcm2835_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct bcm2835_gpio_chip *bcmgpio = container_of(chip, struct bcm2835_gpio_chip, chip);
	void __iomem *base = bcmgpio->base;
	unsigned gpio_bank = gpio / 32;
	unsigned gpio_field_offset = gpio % 32;
	unsigned lev;

	lev = readl(base + GPIOLEV(gpio_bank));
	return 0x1 & (lev >> gpio_field_offset);
}

static int bcm2835_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	return bcm2835_set_function(chip, gpio, GPIO_FSEL_INPUT);
}

static int bcm2835_gpio_direction_output(struct gpio_chip *chip, unsigned gpio, int value)
{
	bcm2835_set_function(chip, gpio, GPIO_FSEL_OUTPUT);
	bcm2835_gpio_set_value(chip, gpio, value);

	return 0;
}

static struct gpio_ops bcm2835_gpio_ops = {
	.direction_input = bcm2835_gpio_direction_input,
	.direction_output = bcm2835_gpio_direction_output,
	.get = bcm2835_gpio_get_value,
	.set = bcm2835_gpio_set_value,
};

static int bcm2835_gpio_probe(struct device_d *dev)
{
	struct bcm2835_gpio_chip *bcmgpio;
	int ret;

	bcmgpio = xzalloc(sizeof(*bcmgpio));
	bcmgpio->base = dev_request_mem_region(dev, 0);
	bcmgpio->chip.ops = &bcm2835_gpio_ops;
	bcmgpio->chip.base = 0;
	bcmgpio->chip.ngpio = 54;
	bcmgpio->chip.dev = dev;

	ret = gpiochip_add(&bcmgpio->chip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip, ret = %d\n", ret);
		goto err;
	}
	dev_info(dev, "probed gpiochip%d with base %d\n", dev->id, bcmgpio->chip.base);

	return 0;

err:
	kfree(bcmgpio);

	return ret;
}

static __maybe_unused struct of_device_id bcm2835_gpio_dt_ids[] = {
	{
		.compatible = "brcm,bcm2835-gpio",
	}, {
		/* sentinel */
	}
};

static struct driver_d bcm2835_gpio_driver = {
	.name = "bcm2835-gpio",
	.probe = bcm2835_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(bcm2835_gpio_dt_ids),
};

static int bcm2835_gpio_add(void)
{
	return platform_driver_register(&bcm2835_gpio_driver);
}
coredevice_initcall(bcm2835_gpio_add);

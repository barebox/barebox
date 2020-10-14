/*
 * Author: Carlo Caione <carlo@carlocaione.org>
 *
 * GPIO code based on linux/arch/arm/mach-bcm2708/bcm2708_gpio.c
 *
 * pinctrl part added by Tomaz Solc <tomaz.solc@tablix.org>
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
#include <pinctrl.h>

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
	struct pinctrl_device pctl;
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

static int bcm2835_pinctrl_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	const __be32 *list;
	u32 function;
	int i, size;

	list = of_get_property(np, "brcm,pins", &size);
	if (!list) {
		return -EINVAL;
	}

	size /= sizeof(*list);

	if (of_property_read_u32(np, "brcm,function", &function)) {
		return -EINVAL;
	}

	for (i = 0; i < size; i++) {
		int pin = be32_to_cpu(list[i]);
		struct bcm2835_gpio_chip *bcmgpio = container_of(pdev, struct bcm2835_gpio_chip, pctl);

		dev_dbg(pdev->dev, "set_state pin %d to function %d\n", pin, function);

		bcm2835_set_function(&bcmgpio->chip, pin, function);
	}

	return 0;
}

static struct pinctrl_ops bcm2835_pinctrl_ops = {
	.set_state = bcm2835_pinctrl_set_state,
};

static int bcm2835_gpio_probe(struct device_d *dev)
{
	struct resource *iores;
	struct bcm2835_gpio_chip *bcmgpio;
	int ret;

	bcmgpio = xzalloc(sizeof(*bcmgpio));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	bcmgpio->base = IOMEM(iores->start);
	bcmgpio->chip.ops = &bcm2835_gpio_ops;
	bcmgpio->chip.base = 0;
	bcmgpio->chip.ngpio = 54;
	bcmgpio->chip.dev = dev;
	bcmgpio->pctl.ops = &bcm2835_pinctrl_ops;
	bcmgpio->pctl.dev = dev;

	ret = gpiochip_add(&bcmgpio->chip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip, ret = %d\n", ret);
		goto err;
	}

	dev_dbg(dev, "probed gpiochip%d with base %d\n", dev->id, bcmgpio->chip.base);

	if (IS_ENABLED(CONFIG_PINCTRL)) {
		ret = pinctrl_register(&bcmgpio->pctl);
		if (ret) {
			dev_err(dev, "couldn't add pinctrl, ret = %d\n", ret);
			// don't free bcmgpio, since it's already used by gpiochip.
		} else {
			dev_dbg(dev, "bcm283x pinctrl registered\n");
		}
	}

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

coredevice_platform_driver(bcm2835_gpio_driver);

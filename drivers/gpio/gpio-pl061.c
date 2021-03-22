/*
 * Copyright (C) 2008, 2009 Provigent Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for the ARM PrimeCell(tm) General Purpose Input/Output (PL061)
 *
 * Data sheet: ARM DDI 0190B, September 2000
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <io.h>
#include <gpio.h>
#include <init.h>

#include <linux/amba/bus.h>
#include <linux/amba/pl061.h>

#define GPIODIR 0x400
#define GPIOIS  0x404
#define GPIOIBE 0x408
#define GPIOIEV 0x40C
#define GPIOIE  0x410
#define GPIORIS 0x414
#define GPIOMIS 0x418
#define GPIOIC  0x41C

#define PL061_GPIO_NR	8

struct pl061_gpio {
	void __iomem		*base;
	struct gpio_chip	gc;
};

static int pl061_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct pl061_gpio *chip = container_of(gc, struct pl061_gpio, gc);
	unsigned char gpiodir;

	if (offset >= gc->ngpio)
		return -EINVAL;

	gpiodir = readb(chip->base + GPIODIR);
	gpiodir &= ~(1 << offset);
	writeb(gpiodir, chip->base + GPIODIR);

	return 0;
}

static int pl061_direction_output(struct gpio_chip *gc, unsigned offset,
		int value)
{
	struct pl061_gpio *chip = container_of(gc, struct pl061_gpio, gc);
	unsigned char gpiodir;

	if (offset >= gc->ngpio)
		return -EINVAL;

	writeb(!!value << offset, chip->base + (1 << (offset + 2)));
	gpiodir = readb(chip->base + GPIODIR);
	gpiodir |= 1 << offset;
	writeb(gpiodir, chip->base + GPIODIR);

	/*
	 * gpio value is set again, because pl061 doesn't allow to set value of
	 * a gpio pin before configuring it in OUT mode.
	 */
	writeb(!!value << offset, chip->base + (1 << (offset + 2)));

	return 0;
}

static int pl061_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct pl061_gpio *chip = container_of(gc, struct pl061_gpio, gc);

	return !!readb(chip->base + (1 << (offset + 2)));
}

static void pl061_set_value(struct gpio_chip *gc, unsigned offset, int value)
{
	struct pl061_gpio *chip = container_of(gc, struct pl061_gpio, gc);

	writeb(!!value << offset, chip->base + (1 << (offset + 2)));
}

static struct gpio_ops pl061_gpio_ops = {
	.direction_input = pl061_direction_input,
	.direction_output = pl061_direction_output,
	.get = pl061_get_value,
	.set = pl061_set_value,
};

static int pl061_probe(struct amba_device *dev, const struct amba_id *id)
{
	struct pl061_platform_data *pdata;
	struct pl061_gpio *chip;
	int ret;

	chip = xzalloc(sizeof(*chip));

	pdata = dev->dev.platform_data;
	if (pdata) {
		chip->gc.base = pdata->gpio_base;
	} else {
		chip->gc.base = -1;
	}

	chip->base = amba_get_mem_region(dev);

	chip->gc.ops = &pl061_gpio_ops;
	chip->gc.ngpio = PL061_GPIO_NR;
	chip->gc.dev = &dev->dev;

	ret = gpiochip_add(&chip->gc);
	if (ret) {
		dev_err(&dev->dev, "couldn't add gpiochip, ret = %d\n", ret);
		goto free_mem;
	}

	writeb(0, chip->base + GPIOIE); /* disable irqs */

	return 0;

free_mem:
	kfree(chip);

	return ret;
}

static struct amba_id pl061_ids[] = {
	{
		.id	= 0x00041061,
		.mask	= 0x000fffff,
	},
	{ 0, 0 },
};

static struct amba_driver pl061_gpio_driver = {
	.drv = {
		.name	= "pl061_gpio",
	},
	.id_table	= pl061_ids,
	.probe		= pl061_probe,
};

static int __init pl061_gpio_init(void)
{
	return amba_driver_register(&pl061_gpio_driver);
}
coredevice_initcall(pl061_gpio_init);

MODULE_AUTHOR("Baruch Siach <baruch@tkos.co.il>");
MODULE_DESCRIPTION("PL061 GPIO driver");
MODULE_LICENSE("GPL");

/*
 * Copyright (C) 2013 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <init.h>
#include <common.h>
#include <malloc.h>

#include <linux/basic_mmio_gpio.h>

static int clps711x_gpio_probe(struct device_d *dev)
{
	int err;
	void __iomem *dat, *dir = NULL, *dir_inv = NULL;
	struct bgpio_chip *bgc;

	if ((dev->id < 0) || (dev->id > 4))
		return -ENODEV;

	dat = dev_request_mem_region(dev, 0);
	switch (dev->id) {
	case 3:
		dir_inv = dev_request_mem_region(dev, 1);
		break;
	default:
		dir = dev_request_mem_region(dev, 1);
		break;
	}

	if (!dat || (!dir && !dir_inv))
		return -EINVAL;

	bgc = xzalloc(sizeof(struct bgpio_chip));
	if (!bgc)
		return -ENOMEM;

	err = bgpio_init(bgc, dev, 1, dat, NULL, NULL, dir, dir_inv, 0);
	if (err) {
		free(bgc);
		return err;
	}

	bgc->gc.base = dev->id * 8;
	switch (dev->id) {
	case 4:
		bgc->gc.ngpio = 3;
		break;
	default:
		bgc->gc.ngpio = 8;
		break;
	}

	return gpiochip_add(&bgc->gc);
}

static struct driver_d clps711x_gpio_driver = {
	.name	= "clps711x-gpio",
	.probe	= clps711x_gpio_probe,
};

static __init int clps711x_gpio_register(void)
{
	return platform_driver_register(&clps711x_gpio_driver);
}
coredevice_initcall(clps711x_gpio_register);

/*
 * bus.c - barebox driver model
 *
 * Copyright (c) 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>

static int platform_probe(struct device_d *dev)
{
	return dev->driver->probe(dev);
}

static void platform_remove(struct device_d *dev)
{
	dev->driver->remove(dev);
}

int platform_driver_register(struct driver_d *drv)
{
	drv->bus = &platform_bus;

	return register_driver(drv);
}

int platform_device_register(struct device_d *new_device)
{
	new_device->bus = &platform_bus;

	return register_device(new_device);
}

struct bus_type platform_bus = {
	.name = "platform",
	.match = device_match,
	.probe = platform_probe,
	.remove = platform_remove,
};

static int platform_init(void)
{
	return bus_register(&platform_bus);
}
pure_initcall(platform_init);

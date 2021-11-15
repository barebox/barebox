// SPDX-License-Identifier: GPL-2.0
/*
 * bus.c - barebox driver model
 *
 * Copyright (c) 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <of.h>
#include <pm_domain.h>

static int platform_probe(struct device_d *dev)
{
	int ret;

	ret = genpd_dev_pm_attach(dev);
	if (ret < 0)
		return ret;

	return dev->driver->probe(dev);
}

static void platform_remove(struct device_d *dev)
{
	if (dev->driver->remove)
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

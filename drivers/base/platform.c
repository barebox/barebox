// SPDX-License-Identifier: GPL-2.0-only
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

static int platform_probe(struct device *dev)
{
	int ret;

	ret = genpd_dev_pm_attach(dev);
	if (ret < 0)
		return dev_err_probe(dev, ret, "power domain attach failed\n");

	return dev->driver->probe(dev);
}

int platform_driver_register(struct driver *drv)
{
	drv->bus = &platform_bus;

	return register_driver(drv);
}
EXPORT_SYMBOL(platform_driver_register);

int platform_device_register(struct device *new_device)
{
	new_device->bus = &platform_bus;

	return register_device(new_device);
}

struct bus_type platform_bus = {
	.name = "platform",
	.match = device_match,
	.probe = platform_probe,
};

static int platform_init(void)
{
	return bus_register(&platform_bus);
}
pure_initcall(platform_init);

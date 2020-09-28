/*
 * fixed regulator support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <malloc.h>
#include <init.h>
#include <regulator.h>
#include <of.h>
#include <of_gpio.h>
#include <gpio.h>

struct regulator_fixed {
	int gpio;
	int active_low;
	int always_on;
	struct regulator_dev rdev;
	struct regulator_desc rdesc;
};

static int regulator_fixed_enable(struct regulator_dev *rdev)
{
	struct regulator_fixed *fix = container_of(rdev, struct regulator_fixed, rdev);

	if (!gpio_is_valid(fix->gpio))
		return 0;

	return gpio_direction_output(fix->gpio, !fix->active_low);
}

static int regulator_fixed_disable(struct regulator_dev *rdev)
{
	struct regulator_fixed *fix = container_of(rdev, struct regulator_fixed, rdev);

	if (fix->always_on)
		return 0;

	if (!gpio_is_valid(fix->gpio))
		return 0;

	return gpio_direction_output(fix->gpio, fix->active_low);
}

const static struct regulator_ops fixed_ops = {
	.enable = regulator_fixed_enable,
	.disable = regulator_fixed_disable,
};

static int regulator_fixed_probe(struct device_d *dev)
{
	struct regulator_fixed *fix;
	enum of_gpio_flags gpioflags;
	int ret;

	if (!dev->device_node)
		return -EINVAL;

	fix = xzalloc(sizeof(*fix));
	fix->gpio = -EINVAL;

	if (of_get_property(dev->device_node, "gpio", NULL)) {
		fix->gpio = of_get_named_gpio_flags(dev->device_node, "gpio", 0, &gpioflags);
		if (fix->gpio < 0) {
			ret = fix->gpio;
			goto err;
		}

		if (gpioflags & OF_GPIO_ACTIVE_LOW)
			fix->active_low = 1;
	}

	fix->rdesc.ops = &fixed_ops;
	fix->rdev.desc = &fix->rdesc;
	fix->rdev.dev = dev;

	if (of_find_property(dev->device_node, "regulator-always-on", NULL) ||
	    of_find_property(dev->device_node, "regulator-boot-on", NULL)) {
		fix->always_on = 1;
		regulator_fixed_enable(&fix->rdev);
	}

	ret = of_regulator_register(&fix->rdev, dev->device_node);
	if (ret)
		return ret;

	return 0;
err:
	free(fix);

	return ret;
}

static struct of_device_id regulator_fixed_of_ids[] = {
	{ .compatible = "regulator-fixed", },
	{ }
};

static struct driver_d regulator_fixed_driver = {
	.name  = "regulator-fixed",
	.probe = regulator_fixed_probe,
	.of_compatible = DRV_OF_COMPAT(regulator_fixed_of_ids),
};
coredevice_platform_driver(regulator_fixed_driver);

// SPDX-License-Identifier: GPL-2.0-only
/*
 * fixed regulator support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <regulator.h>
#include <of.h>
#include <linux/gpio/consumer.h>

struct regulator_fixed {
	struct gpio_desc *gpio;
	struct regulator_dev rdev;
	struct regulator_desc rdesc;
};

static int regulator_fixed_enable(struct regulator_dev *rdev)
{
	struct regulator_fixed *fix = container_of(rdev, struct regulator_fixed, rdev);

	return gpiod_direction_output(fix->gpio, true);
}

static int regulator_fixed_disable(struct regulator_dev *rdev)
{
	struct regulator_fixed *fix = container_of(rdev, struct regulator_fixed, rdev);

	return gpiod_direction_output(fix->gpio, false);
}

const static struct regulator_ops fixed_ops = {
	.enable = regulator_fixed_enable,
	.disable = regulator_fixed_disable,
};

static int regulator_fixed_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct regulator_fixed *fix;
	u32 delay;
	int ret;

	if (!dev->of_node)
		return -EINVAL;

	fix = xzalloc(sizeof(*fix));

	fix->gpio = gpiod_get_optional(dev, NULL, GPIOD_ASIS);
	if (IS_ERR(fix->gpio)) {
		ret = PTR_ERR(fix->gpio);
		goto err;
	}

	fix->rdesc.ops = &fixed_ops;
	fix->rdev.desc = &fix->rdesc;
	fix->rdev.dev = dev;

	if (!of_property_read_u32(np, "off-on-delay-us", &delay))
		fix->rdesc.off_on_delay = delay;

	if (of_find_property(np, "vin-supply", NULL))
		fix->rdesc.supply_name = "vin";

	ret = of_regulator_register(&fix->rdev, np);
	if (ret)
		goto err;

	return 0;
err:
	free(fix);

	return ret;
}

static struct of_device_id regulator_fixed_of_ids[] = {
	{ .compatible = "regulator-fixed", },
	{ }
};
MODULE_DEVICE_TABLE(of, regulator_fixed_of_ids);

static struct driver regulator_fixed_driver = {
	.name  = "regulator-fixed",
	.probe = regulator_fixed_probe,
	.of_compatible = DRV_OF_COMPAT(regulator_fixed_of_ids),
};
coredevice_platform_driver(regulator_fixed_driver);

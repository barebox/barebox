// SPDX-License-Identifier: GPL-2.0-only
/*
 * I2C driver for the X-Powers' Power Management ICs
 *
 * AXP20x typically comprises an adaptive USB-Compatible PWM charger, BUCK DC-DC
 * converters, LDOs, multiple 12-bit ADCs of voltage, current and temperature
 * as well as configurable GPIOs.
 *
 * This driver supports the I2C variants.
 *
 * Copyright (C) 2014 Carlo Caione
 *
 * Author: Carlo Caione <carlo@caione.org>
 */

#include <common.h>
#include <of.h>
#include <linux/err.h>
#include <i2c/i2c.h>
#include <module.h>
#include <linux/mfd/axp20x.h>
#include <regmap.h>

static int axp20x_i2c_probe(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct axp20x_dev *axp20x;
	int ret;

	axp20x = xzalloc(sizeof(*axp20x));

	axp20x->dev = dev;

	ret = axp20x_match_device(axp20x);
	if (ret)
		return ret;

	axp20x->regmap = regmap_init_i2c(client, axp20x->regmap_cfg);
	if (IS_ERR(axp20x->regmap))
		return dev_err_probe(dev, PTR_ERR(axp20x->regmap),
				     "regmap init failed\n");

	ret = axp20x_device_probe(axp20x);
	if (ret)
		return ret;

	return regmap_register_cdev(axp20x->regmap, NULL);
}

static const struct of_device_id axp20x_i2c_of_match[] = {
	{ .compatible = "x-powers,axp152", .data = (void *)AXP152_ID },
	{ .compatible = "x-powers,axp202", .data = (void *)AXP202_ID },
	{ .compatible = "x-powers,axp209", .data = (void *)AXP209_ID },
	{ .compatible = "x-powers,axp313a", .data = (void *)AXP313A_ID },
	{ .compatible = "x-powers,axp221", .data = (void *)AXP221_ID },
	{ .compatible = "x-powers,axp223", .data = (void *)AXP223_ID },
	{ .compatible = "x-powers,axp803", .data = (void *)AXP803_ID },
	{ .compatible = "x-powers,axp806", .data = (void *)AXP806_ID },
	{ },
};
MODULE_DEVICE_TABLE(of, axp20x_i2c_of_match);

static struct driver axp20x_i2c_driver = {
	.name		= "axp20x-i2c",
	.probe		= axp20x_i2c_probe,
	.of_compatible	= DRV_OF_COMPAT(axp20x_i2c_of_match),
};

coredevice_i2c_driver(axp20x_i2c_driver);

MODULE_DESCRIPTION("PMIC MFD I2C driver for AXP20X");
MODULE_AUTHOR("Carlo Caione <carlo@caione.org>");
MODULE_LICENSE("GPL");

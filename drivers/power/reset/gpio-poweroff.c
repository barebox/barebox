// SPDX-License-Identifier: GPL-2.0-only
/*
 * Toggles a GPIO pin to power down a device
 *
 * Jamie Lentin <jm@lentin.co.uk>
 * Andrew Lunn <andrew@lunn.ch>
 *
 * Copyright (C) 2012 Jamie Lentin
 */
#include <common.h>
#include <driver.h>
#include <poweroff.h>
#include <gpiod.h>

#define DEFAULT_TIMEOUT_MS 3000
/*
 * Hold configuration here, cannot be more than one instance of the driver
 * since pm_power_off itself is global.
 */
static int reset_gpio;
static u32 timeout = DEFAULT_TIMEOUT_MS;
static u32 active_delay = 100;
static u32 inactive_delay = 100;

static void gpio_poweroff_do_poweroff(struct poweroff_handler *handler)
{
	/* drive it active, also inactive->active edge */
	gpio_direction_active(reset_gpio, true);
	mdelay(active_delay);

	/* drive inactive, also active->inactive edge */
	gpio_set_active(reset_gpio, false);
	mdelay(inactive_delay);

	/* drive it active, also inactive->active edge */
	gpio_set_active(reset_gpio, true);

	/* give it some time */
	mdelay(timeout);

	WARN_ON(1);
}

static struct poweroff_handler handler;

static int gpio_poweroff_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	bool input = false;
	enum gpiod_flags flags;

	if (handler.poweroff != NULL) {
		dev_err(dev, "%s: pm_power_off function already registered\n", __func__);
		return -EBUSY;
	}

	input = of_property_read_bool(np, "input");
	if (input)
		flags = GPIOD_IN;
	else
		flags = GPIOD_OUT_LOW;

	of_property_read_u32(np, "active-delay-ms", &active_delay);
	of_property_read_u32(np, "inactive-delay-ms", &inactive_delay);
	of_property_read_u32(np, "timeout-ms", &timeout);

	reset_gpio = gpiod_get(dev, NULL, flags);
	if (reset_gpio < 0)
		return reset_gpio;

	handler.name = "gpio-poweroff";
	handler.poweroff = gpio_poweroff_do_poweroff;
	handler.priority = 129;

	return poweroff_handler_register(&handler);
}

static const struct of_device_id of_gpio_poweroff_match[] = {
	{ .compatible = "gpio-poweroff", },
	{},
};
MODULE_DEVICE_TABLE(of, of_gpio_poweroff_match);

static struct driver gpio_poweroff_driver = {
	.name = "poweroff-gpio",
	.of_compatible = of_gpio_poweroff_match,
	.probe = gpio_poweroff_probe,
};
device_platform_driver(gpio_poweroff_driver);

MODULE_AUTHOR("Jamie Lentin <jm@lentin.co.uk>");
MODULE_DESCRIPTION("GPIO poweroff driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:poweroff-gpio");

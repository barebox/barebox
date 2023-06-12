// SPDX-License-Identifier: GPL-2.0-only
/*
 * Toggles a GPIO pin to restart a device
 *
 * Copyright (C) 2014 Google, Inc.
 *
 * Based on the gpio-poweroff driver.
 */
#include <common.h>
#include <driver.h>
#include <restart.h>
#include <gpiod.h>

struct gpio_restart {
	int reset_gpio;
	struct restart_handler restart_handler;
	u32 active_delay_ms;
	u32 inactive_delay_ms;
	u32 wait_delay_ms;
};

static void __noreturn gpio_restart_handle(struct restart_handler *this)
{
	struct gpio_restart *gpio_restart =
		container_of(this, struct gpio_restart, restart_handler);

	/* drive it active, also inactive->active edge */
	gpio_direction_active(gpio_restart->reset_gpio, true);
	mdelay(gpio_restart->active_delay_ms);

	/* drive inactive, also active->inactive edge */
	gpio_set_active(gpio_restart->reset_gpio, false);
	mdelay(gpio_restart->inactive_delay_ms);

	/* drive it active, also inactive->active edge */
	gpio_set_active(gpio_restart->reset_gpio, true);

	/* give it some time */
	mdelay(gpio_restart->wait_delay_ms);

	mdelay(1000);

	panic("Unable to restart system\n");
}

static int gpio_restart_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct gpio_restart *gpio_restart;
	bool open_source = false;
	u32 property;
	int ret;

	gpio_restart = xzalloc(sizeof(*gpio_restart));

	open_source = of_property_read_bool(np, "open-source");

	gpio_restart->reset_gpio = gpiod_get(dev, NULL,
			open_source ? GPIOD_IN : GPIOD_OUT_LOW);
	ret = gpio_restart->reset_gpio;
	if (ret < 0) {
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Could not get reset GPIO\n");
		return ret;
	}

	gpio_restart->restart_handler.restart = gpio_restart_handle;
	gpio_restart->restart_handler.name = "gpio-restart";
	gpio_restart->restart_handler.priority = 129;
	gpio_restart->active_delay_ms = 100;
	gpio_restart->inactive_delay_ms = 100;
	gpio_restart->wait_delay_ms = 3000;

	ret = of_property_read_u32(dev->of_node, "priority", &property);
	if (!ret)
		gpio_restart->restart_handler.priority = property;

	of_property_read_u32(np, "active-delay", &gpio_restart->active_delay_ms);
	of_property_read_u32(np, "inactive-delay", &gpio_restart->inactive_delay_ms);
	of_property_read_u32(np, "wait-delay", &gpio_restart->wait_delay_ms);

	return restart_handler_register(&gpio_restart->restart_handler);
}

static const struct of_device_id of_gpio_restart_match[] = {
	{ .compatible = "gpio-restart", },
	{},
};
MODULE_DEVICE_TABLE(of, of_gpio_restart_match);

static struct driver gpio_restart_driver = {
	.name = "restart-gpio",
	.of_compatible = of_gpio_restart_match,
	.probe = gpio_restart_probe,
};
device_platform_driver(gpio_restart_driver);

MODULE_AUTHOR("David Riley <davidriley@chromium.org>");
MODULE_DESCRIPTION("GPIO restart driver");
MODULE_LICENSE("GPL");

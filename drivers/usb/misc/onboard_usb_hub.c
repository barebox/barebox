// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for onboard USB hubs
 *
 * Copyright (c) 2022, Google LLC
 */

#include <driver.h>
#include <linux/gpio/consumer.h>
#include <init.h>
#include <of.h>
#include <linux/printk.h>
#include <of_device.h>
#include <regulator.h>
#include <linux/usb/usb.h>

#include "onboard_usb_hub.h"

void of_usb_host_probe_hubs(struct usb_host *host)
{
	struct device_node *np;

	np = dev_of_node(host->hw_dev);
	if (!np)
		return;

	of_platform_populate(np, onboard_hub_match, host->hw_dev);
}

struct onboard_hub {
	struct regulator *vdd;
	struct device *dev;
	const struct onboard_hub_pdata *pdata;
	struct gpio_desc *reset_gpio;
};

static int onboard_hub_power_on(struct onboard_hub *hub)
{
	int err;

	err = regulator_enable(hub->vdd);
	if (err) {
		dev_err(hub->dev, "failed to enable regulator: %pe\n",
			ERR_PTR(err));
		return err;
	}

	udelay(hub->pdata->reset_us);
	gpiod_set_value(hub->reset_gpio, 0);

	return 0;
}

static int onboard_hub_probe(struct device *dev)
{
	struct onboard_hub *hub;

	hub = xzalloc(sizeof(*hub));

	hub->pdata = device_get_match_data(dev);
	if (!hub->pdata)
		return -EINVAL;

	hub->vdd = regulator_get(dev, "vdd");
	if (IS_ERR(hub->vdd))
		return PTR_ERR(hub->vdd);

	hub->reset_gpio = gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(hub->reset_gpio))
		return dev_errp_probe(dev, hub->reset_gpio,
				     "failed to get reset GPIO\n");

	hub->dev = dev;

	return onboard_hub_power_on(hub);
}

static struct driver onboard_hub_driver = {
	.name = "onboard-usb-hub",
	.probe = onboard_hub_probe,
	.of_compatible = onboard_hub_match,
};
device_platform_driver(onboard_hub_driver);

MODULE_AUTHOR("Matthias Kaehlcke <mka@chromium.org>");
MODULE_DESCRIPTION("Driver for discrete onboard USB hubs");
MODULE_LICENSE("GPL v2");

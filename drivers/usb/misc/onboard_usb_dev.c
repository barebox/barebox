// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for onboard USB devices
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
#include <xfuncs.h>
#include <linux/usb/usb.h>

#include "onboard_usb_dev.h"

void of_usb_host_probe_devs(struct usb_host *host)
{
	struct device_node *np;

	np = dev_of_node(host->hw_dev);
	if (!np)
		return;

	of_platform_populate(np, onboard_dev_match, host->hw_dev);
}

struct onboard_dev {
	struct regulator *vdd;
	struct device *dev;
	const struct onboard_dev_pdata *pdata;
	struct gpio_desc *reset_gpio;
};

static int onboard_dev_power_on(struct onboard_dev *onboard_dev)
{
	int err;

	err = regulator_enable(onboard_dev->vdd);
	if (err) {
		dev_err(onboard_dev->dev, "failed to enable regulator: %pe\n",
			ERR_PTR(err));
		return err;
	}

	udelay(onboard_dev->pdata->reset_us);
	gpiod_set_value(onboard_dev->reset_gpio, 0);

	return 0;
}

static int onboard_dev_probe(struct device *dev)
{
	struct device_node *peer_node;
	struct device *peer_dev;
	struct onboard_dev *onboard_dev;

	peer_node = of_parse_phandle(dev->of_node, "peer-hub", 0);
	if (peer_node) {
		peer_dev = of_find_device_by_node(peer_node);
		if (peer_dev && peer_dev->priv)
			return 0;
	}

	onboard_dev = xzalloc(sizeof(*onboard_dev));

	onboard_dev->pdata = device_get_match_data(dev);
	if (!onboard_dev->pdata)
		return -EINVAL;

	onboard_dev->vdd = regulator_get(dev, "vdd");
	if (IS_ERR(onboard_dev->vdd))
		return PTR_ERR(onboard_dev->vdd);

	onboard_dev->reset_gpio = gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(onboard_dev->reset_gpio))
		return dev_errp_probe(dev, onboard_dev->reset_gpio,
				     "failed to get reset GPIO\n");

	onboard_dev->dev = dev;
	dev->priv = onboard_dev;

	return onboard_dev_power_on(onboard_dev);
}

static struct driver onboard_dev_driver = {
	.name = "onboard-usb-dev",
	.probe = onboard_dev_probe,
	.of_compatible = onboard_dev_match,
};
device_platform_driver(onboard_dev_driver);

MODULE_AUTHOR("Matthias Kaehlcke <mka@chromium.org>");
MODULE_DESCRIPTION("Driver for discrete onboard USB devices");
MODULE_LICENSE("GPL v2");

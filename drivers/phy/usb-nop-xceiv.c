/*
 * Copyright (c) 2016 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <common.h>
#include <init.h>
#include <of.h>
#include <errno.h>
#include <driver.h>
#include <malloc.h>
#include <usb/phy.h>
#include <linux/phy/phy.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <gpio.h>
#include <of_gpio.h>

struct nop_usbphy {
	struct usb_phy usb_phy;
	struct phy *phy;
	struct phy_provider *provider;
	struct clk *clk;
	int reset;
};

static struct phy *nop_usbphy_xlate(struct device_d *dev,
				    struct of_phandle_args *args)
{
	struct nop_usbphy *nopphy = dev->priv;

	return nopphy->phy;
}

static int nop_usbphy_init(struct phy *phy)
{
	int ret;
	struct nop_usbphy *nopphy = phy_get_drvdata(phy);

	ret = clk_enable(nopphy->clk);
	if (ret < 0)
		return ret;

	if (gpio_is_valid(nopphy->reset)) {
		/*
		 * Let's wait for 100 ms before deasserting reset line
		 */
		mdelay(100);
		gpio_set_active(nopphy->reset, false);
	}

	return 0;
}

static struct usb_phy *nop_usbphy_to_usbphy(struct phy *phy)
{
	struct nop_usbphy *nopphy = phy_get_drvdata(phy);

	return &nopphy->usb_phy;
}

static const struct phy_ops nop_phy_ops = {
	.to_usbphy = nop_usbphy_to_usbphy,
	.init = nop_usbphy_init,
};

static int nop_usbphy_probe(struct device_d *dev)
{
	int ret;
	struct nop_usbphy *nopphy;
	enum of_gpio_flags of_flags;
	char *name = NULL;
	unsigned long flags = GPIOF_OUT_INIT_ACTIVE;

	nopphy = xzalloc(sizeof(*nopphy));

	dev->priv = nopphy;

	nopphy->clk = clk_get(dev, "main_clk");
	if (IS_ERR(nopphy->clk))
		nopphy->clk = NULL;

	nopphy->reset = of_get_named_gpio_flags(dev->device_node,
						"reset-gpios", 0, &of_flags);
	if (gpio_is_valid(nopphy->reset)) {
		/* assert reset */

		if (of_flags & OF_GPIO_ACTIVE_LOW)
			flags |= GPIOF_ACTIVE_LOW;

		name = basprintf("%s reset", dev_name(dev));
		ret  = gpio_request_one(nopphy->reset, flags, name);
		if (ret < 0)
			goto err_free;
	}

	/* FIXME: Add vbus regulator support */
	/* FIXME: Add vbus-detect-gpio support */

	nopphy->usb_phy.dev = dev;
	nopphy->phy = phy_create(dev, NULL, &nop_phy_ops, NULL);
	if (IS_ERR(nopphy->phy)) {
		ret = PTR_ERR(nopphy->phy);
		goto release_gpio;
	}

	phy_set_drvdata(nopphy->phy, nopphy);

	nopphy->provider = of_phy_provider_register(dev, nop_usbphy_xlate);
	if (IS_ERR(nopphy->provider)) {
		ret = PTR_ERR(nopphy->provider);
		goto release_gpio;
	}

	return 0;

release_gpio:
	if (gpio_is_valid(nopphy->reset))
		gpio_free(nopphy->reset);
err_free:
	free(nopphy);
	free(name);
	return ret;
};

static __maybe_unused struct of_device_id nop_usbphy_dt_ids[] = {
	{
		.compatible = "usb-nop-xceiv",
	}, {
		/* sentinel */
	},
};

static struct driver_d nop_usbphy_driver = {
	.name   = "usb-nop-xceiv",
	.probe  = nop_usbphy_probe,
	.of_compatible = DRV_OF_COMPAT(nop_usbphy_dt_ids),
};

static int nop_usbphy_driver_init(void)
{
	return platform_driver_register(&nop_usbphy_driver);
}
fs_initcall(nop_usbphy_driver_init);

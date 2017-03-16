/*
 * (C) Copyright 2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <linux/clk.h>
#include <driver.h>
#include <init.h>
#include <usb/usb.h>
#include <usb/usb_defs.h>
#include <errno.h>
#include <gpio.h>
#include <of_gpio.h>
#include <io.h>

#include <mach/board.h>

#include "ohci.h"

#define at91_for_each_port(index)					\
	for ((index) = 0; (index) < AT91_MAX_USBH_PORTS; (index)++)


struct ohci_at91_priv {
	struct device_d *dev;
	struct clk *iclk;
	struct clk *fclk;
	struct ohci_regs __iomem *regs;
};

static int at91_start_clock(struct ohci_at91_priv *ohci_at91)
{
	int ret;

	ret = clk_enable(ohci_at91->iclk);
	if (ret < 0) {
		dev_err(ohci_at91->dev, "Failed to enable 'iclk'\n");
		return ret;
	}

	ret = clk_enable(ohci_at91->fclk);
	if (ret < 0) {
		dev_err(ohci_at91->dev, "Failed to enable 'fclk'\n");
		return ret;
	}

	return 0;
}

static void at91_stop_clock(struct ohci_at91_priv *ohci_at91)
{
	clk_disable(ohci_at91->fclk);
	clk_disable(ohci_at91->iclk);
}

static int at91_ohci_probe_dt(struct device_d *dev)
{
	u32 ports;
	int i, ret, gpio;
	enum of_gpio_flags flags;
	struct at91_usbh_data *pdata;
	struct device_node *np = dev->device_node;

	pdata = xzalloc(sizeof(*pdata));
	dev->platform_data = pdata;

	if (!of_property_read_u32(np, "num-ports", &ports)) {
		pdata->ports = ports;
	} else {
		dev_err(dev, "Failed to read 'num-ports' property\n");
		return -EINVAL;
	}

	at91_for_each_port(i) {
		/*
		 * do not configure PIO if not in relation with
		 * real USB port on board
		 */
		if (i >= pdata->ports) {
			pdata->vbus_pin[i] = -EINVAL;
			continue;
		}

		gpio = of_get_named_gpio_flags(np, "atmel,vbus-gpio", i,
					       &flags);
		pdata->vbus_pin[i] = gpio;
		if (!gpio_is_valid(gpio))
			continue;
		pdata->vbus_pin_active_low[i] = flags & OF_GPIO_ACTIVE_LOW;

		ret = gpio_request(gpio, "ohci_vbus");
		if (ret) {
			dev_err(dev, "can't request vbus gpio %d\n", gpio);
			continue;
		}
		ret = gpio_direction_output(gpio,
					    !pdata->vbus_pin_active_low[i]);
		if (ret) {
			dev_err(dev, "can't put vbus gpio %d as output %d\n",
				gpio, !pdata->vbus_pin_active_low[i]);
			gpio_free(gpio);
			continue;
		}
	}

	return 0;
}

static int at91_ohci_probe(struct device_d *dev)
{
	int ret;
	struct resource *io;
	struct ohci_at91_priv *ohci_at91 = xzalloc(sizeof(*ohci_at91));

	dev->priv = ohci_at91;
	ohci_at91->dev = dev;

	if (dev->device_node) {
		ret = at91_ohci_probe_dt(dev);
		if (ret < 0)
			return ret;
	}

	io = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(io)) {
		dev_err(dev, "Failed to get IORESOURCE_MEM\n");
		return PTR_ERR(io);
	}
	ohci_at91->regs = IOMEM(io->start);

	ohci_at91->iclk = clk_get(dev, "ohci_clk");
	if (IS_ERR(ohci_at91->iclk)) {
		dev_err(dev, "Failed to get 'ohci_clk'\n");
		return PTR_ERR(ohci_at91->iclk);
	}

	ohci_at91->fclk = clk_get(dev, "uhpck");
	if (IS_ERR(ohci_at91->fclk)) {
		dev_err(dev, "Failed to get 'uhpck'\n");
		return PTR_ERR(ohci_at91->fclk);
	}

	/*
	 * Start the USB clocks.
	 */
	ret = at91_start_clock(ohci_at91);
	if (ret < 0)
		return ret;

	/*
	 * The USB host controller must remain in reset.
	 */
	writel(0, &ohci_at91->regs->control);

	add_generic_device("ohci", DEVICE_ID_DYNAMIC, NULL, io->start,
			   resource_size(io), IORESOURCE_MEM, NULL);

	return 0;
}

static void at91_ohci_remove(struct device_d *dev)
{
	struct at91_usbh_data *pdata = dev->platform_data;
	struct ohci_at91_priv *ohci_at91 = dev->priv;

	/*
	 * Put the USB host controller into reset.
	 */
	writel(0, &ohci_at91->regs->control);

	/*
	 * Stop the USB clocks.
	 */
	at91_stop_clock(ohci_at91);

	if (pdata) {
		bool active_low;
		int  i, gpio;

		at91_for_each_port(i) {
			gpio = pdata->vbus_pin[i];
			active_low = pdata->vbus_pin_active_low[i];

			if (gpio_is_valid(gpio)) {
				gpio_set_value(gpio, active_low);
				gpio_free(gpio);
			}
		}
	}
}

static const struct of_device_id at91_ohci_dt_ids[] = {
	{ .compatible = "atmel,at91rm9200-ohci" },
	{ /* sentinel */ }
};

static struct driver_d at91_ohci_driver = {
	.name = "at91_ohci",
	.probe = at91_ohci_probe,
	.remove = at91_ohci_remove,
	.of_compatible = DRV_OF_COMPAT(at91_ohci_dt_ids),
};
device_platform_driver(at91_ohci_driver);

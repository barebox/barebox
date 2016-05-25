/*
 * Copyright (C) 2014 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>

struct ohci_platform_data {
	int (*probe)(struct device_d *, struct resource *);
};

static int at91_ohci_probe(struct device_d *dev, struct resource *res)
{
	struct clk *iclk, *fclk;

	iclk = clk_get(NULL, "ohci_clk");
	if (IS_ERR(iclk))
		return PTR_ERR(iclk);

	fclk = clk_get(NULL, "uhpck");
	if (IS_ERR(fclk))
		return PTR_ERR(fclk);

	/* Start the USB clocks */
	clk_enable(iclk);
	clk_enable(fclk);

	return 0;
}

static struct ohci_platform_data at91_ohci_data = {
	.probe	= at91_ohci_probe,
};

static struct platform_device_id ohci_platform_ids[] = {
	{
		.name		= "at91_ohci",
		.driver_data	= (unsigned long)&at91_ohci_data,
	},
	{ }
};

static int ohci_platform_probe(struct device_d *dev)
{
	struct ohci_platform_data *data;
	struct resource *res;
	int ret;

	ret = dev_get_drvdata(dev, (unsigned long *)&data);
	if (ret)
		return ret;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	dev->priv = data;

	if (data->probe) {
		ret = data->probe(dev, res);
		if (ret)
			return ret;
	}

	add_generic_device_res("ohci", DEVICE_ID_SINGLE, res, 1, NULL);

	return 0;
}

static struct driver_d ohci_platform_driver = {
	.name		= "ohci-platform",
	.id_table	= ohci_platform_ids,
	.probe		= ohci_platform_probe,
};
device_platform_driver(ohci_platform_driver);

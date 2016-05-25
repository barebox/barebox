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

static struct platform_device_id ohci_platform_ids[] = {
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

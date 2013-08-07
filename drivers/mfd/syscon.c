/* System Control Driver
 *
 * Based on linux driver by:
 *  Copyright (C) 2012 Freescale Semiconductor, Inc.
 *  Copyright (C) 2012 Linaro Ltd.
 *  Author: Dong Aisheng <dong.aisheng@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <io.h>
#include <init.h>
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>

#include <linux/err.h>

#include <mfd/syscon.h>

struct syscon {
	void __iomem *base;
};

void __iomem *syscon_base_lookup_by_pdevname(const char *s)
{
	struct syscon *syscon;
	struct device_d *dev;

	for_each_device(dev) {
		if (!strcmp(dev_name(dev), s)) {
			syscon = dev->priv;
			return syscon->base;
		}
	}

	return ERR_PTR(-ENODEV);
}

static int syscon_probe(struct device_d *dev)
{
	struct syscon *syscon;
	struct resource *res;

	syscon = xzalloc(sizeof(struct syscon));
	if (!syscon)
		return -ENOMEM;

	res = dev_get_resource(dev, 0);
	if (!res) {
		free(syscon);
		return -ENOENT;
	}

	res = request_iomem_region(dev_name(dev), res->start, res->end);
	if (!res) {
		free(syscon);
		return -EBUSY;
	}

	syscon->base = (void __iomem *)res->start;
	dev->priv = syscon;

	dev_dbg(dev, "map 0x%x-0x%x registered\n", res->start, res->end);

	return 0;
}

static struct platform_device_id syscon_ids[] = {
	{ "syscon", },
#ifdef CONFIG_ARCH_CLPS711X
	{ "clps711x-syscon", },
#endif
	{ }
};

static struct driver_d syscon_driver = {
	.name		= "syscon",
	.probe		= syscon_probe,
	.id_table	= syscon_ids,
};

static int __init syscon_init(void)
{
	return platform_driver_register(&syscon_driver);
}
core_initcall(syscon_init);

MODULE_AUTHOR("Dong Aisheng <dong.aisheng@linaro.org>");
MODULE_DESCRIPTION("System Control driver");
MODULE_LICENSE("GPL v2");

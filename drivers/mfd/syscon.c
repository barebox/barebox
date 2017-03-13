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
#include <of_address.h>
#include <linux/err.h>

#include <mfd/syscon.h>

static LIST_HEAD(syscon_list);

struct syscon {
	struct device_node *np;
	void __iomem *base;
	struct list_head list;
	struct regmap *regmap;
};

static int syscon_reg_write(void *context, unsigned int reg,
			    unsigned int val)
{
	struct syscon *syscon = context;
	writel(val, syscon->base + reg);
	return 0;
}

static int syscon_reg_read(void *context, unsigned int reg,
			   unsigned int *val)
{
	struct syscon *syscon = context;
	*val = readl(syscon->base + reg);
	return 0;
}

static const struct regmap_bus syscon_regmap_bus = {
	.reg_write = syscon_reg_write,
	.reg_read  = syscon_reg_read,
};

static const struct regmap_config syscon_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static struct syscon *of_syscon_register(struct device_node *np)
{
	int ret;
	struct syscon *syscon;
	struct resource res;

	if (!of_device_is_compatible(np, "syscon"))
		return ERR_PTR(-EINVAL);

	syscon = xzalloc(sizeof(*syscon));

	if (of_address_to_resource(np, 0, &res)) {
		ret = -ENOMEM;
		goto err_map;
	}

	syscon->base = IOMEM(res.start);
	syscon->np   = np;

	list_add_tail(&syscon->list, &syscon_list);

	syscon->regmap = regmap_init(NULL,
				     &syscon_regmap_bus,
				     syscon,
				     &syscon_regmap_config);
	return syscon;

err_map:
	kfree(syscon);
	return ERR_PTR(ret);
}

static struct syscon *node_to_syscon(struct device_node *np)
{
	struct syscon *entry, *syscon = NULL;

	list_for_each_entry(entry, &syscon_list, list)
		if (entry->np == np) {
			syscon = entry;
			break;
		}

	if (!syscon)
		syscon = of_syscon_register(np);

	if (IS_ERR(syscon))
		return ERR_CAST(syscon);

	return syscon;
}

static void __iomem *syscon_node_to_base(struct device_node *np)
{
	struct syscon *syscon = node_to_syscon(np);

	if (IS_ERR(syscon))
		return ERR_CAST(syscon);

	return syscon->base;
}

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

void __iomem *syscon_base_lookup_by_phandle(struct device_node *np,
					    const char *property)
{
	struct device_node *syscon_np;

	if (property)
		syscon_np = of_parse_phandle(np, property, 0);
	else
		syscon_np = np;

	if (!syscon_np)
		return ERR_PTR(-ENODEV);

	return syscon_node_to_base(syscon_np);
}

struct regmap *syscon_node_to_regmap(struct device_node *np)
{
	struct syscon *syscon = node_to_syscon(np);

	if (IS_ERR(syscon))
		return ERR_CAST(syscon);

	return syscon->regmap;
}

static int syscon_probe(struct device_d *dev)
{
	struct syscon *syscon;
	struct resource *res;

	syscon = xzalloc(sizeof(struct syscon));

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res)) {
		free(syscon);
		return PTR_ERR(res);
	}

	syscon->base = IOMEM(res->start);
	dev->priv = syscon;

	dev_dbg(dev, "map 0x%x-0x%x registered\n", res->start, res->end);

	return 0;
}

static struct platform_device_id syscon_ids[] = {
	{ "syscon", },
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

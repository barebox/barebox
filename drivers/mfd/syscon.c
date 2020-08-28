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
#include <linux/clk.h>

#include <mfd/syscon.h>

static LIST_HEAD(syscon_list);

struct syscon {
	struct device_node *np;
	void __iomem *base;
	struct list_head list;
	struct regmap *regmap;
};

static const struct regmap_config syscon_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static struct syscon *of_syscon_register(struct device_node *np, bool check_clk)
{
	struct regmap_config syscon_config = syscon_regmap_config;
	struct syscon *syscon;
	u32 reg_io_width;
	int ret;
	struct resource res;

	syscon = xzalloc(sizeof(*syscon));

	if (of_address_to_resource(np, 0, &res)) {
		ret = -ENOMEM;
		goto err_map;
	}

	syscon->base = IOMEM(res.start);

	/* Parse the device's DT node for an endianness specification */
	if (of_property_read_bool(np, "big-endian"))
		syscon_config.val_format_endian = REGMAP_ENDIAN_BIG;
	else if (of_property_read_bool(np, "little-endian"))
		syscon_config.val_format_endian = REGMAP_ENDIAN_LITTLE;
	else if (of_property_read_bool(np, "native-endian"))
		syscon_config.val_format_endian = REGMAP_ENDIAN_NATIVE;

	/*
	 * search for reg-io-width property in DT. If it is not provided,
	 * default to 4 bytes. regmap_init_mmio will return an error if values
	 * are invalid so there is no need to check them here.
	 */
	ret = of_property_read_u32(np, "reg-io-width", &reg_io_width);
	if (ret)
		reg_io_width = 4;

	syscon_config.name = np->full_name;
	syscon_config.reg_stride = reg_io_width;
	syscon_config.val_bits = reg_io_width * 8;
	syscon_config.max_register = resource_size(&res) - reg_io_width;

	list_add_tail(&syscon->list, &syscon_list);

	syscon->regmap = regmap_init_mmio_clk(NULL, NULL, syscon->base,
					      &syscon_config);

	if (check_clk) {
		struct clk *clk = of_clk_get(np, 0);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			/* clock is optional */
			if (ret != -ENOENT)
				goto err_map;
		} else {
			ret = regmap_mmio_attach_clk(syscon->regmap, clk);
			if (ret)
				goto err_map;
		}
	}

	return syscon;

err_map:
	kfree(syscon);
	return ERR_PTR(ret);
}

static struct syscon *node_to_syscon(struct device_node *np, bool check_clk)
{
	struct syscon *entry, *syscon = NULL;

	list_for_each_entry(entry, &syscon_list, list)
		if (entry->np == np) {
			syscon = entry;
			break;
		}

	if (!syscon)
		syscon = of_syscon_register(np, check_clk);

	if (IS_ERR(syscon))
		return ERR_CAST(syscon);

	return syscon;
}

static void __iomem *syscon_node_to_base(struct device_node *np)
{
	struct syscon *syscon;
	struct clk *clk;

	if (!of_device_is_compatible(np, "syscon"))
		return ERR_PTR(-EINVAL);

	syscon = node_to_syscon(np, true);
	if (IS_ERR(syscon))
		return ERR_CAST(syscon);

	/* Returning the direct pointer here side steps the regmap
	 * and any specified clock wouldn't be enabled on access.
	 * Deal with this by enabling the clock permanently if any
	 * syscon_node_to_base users exist.
	 */
	clk = of_clk_get(np, 0);
	if (!IS_ERR(clk))
		clk_enable(clk);

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

static struct regmap *__device_node_to_regmap(struct device_node *np,
					      bool check_clk)
{
	struct syscon *syscon;

	syscon = node_to_syscon(np, check_clk);
	if (IS_ERR(syscon))
		return ERR_CAST(syscon);

	return syscon->regmap;
}

struct regmap *device_node_to_regmap(struct device_node *np)
{
	return __device_node_to_regmap(np, false);
}

struct regmap *syscon_node_to_regmap(struct device_node *np)
{
	if (!of_device_is_compatible(np, "syscon"))
		return ERR_PTR(-EINVAL);

	return __device_node_to_regmap(np, true);
}

struct regmap *syscon_regmap_lookup_by_compatible(const char *s)
{
	struct device_node *syscon_np;
	struct regmap *regmap;

	syscon_np = of_find_compatible_node(NULL, NULL, s);
	if (!syscon_np)
		return ERR_PTR(-ENODEV);

	regmap = syscon_node_to_regmap(syscon_np);

	return regmap;
}

struct regmap *syscon_regmap_lookup_by_phandle(struct device_node *np,
					const char *property)
{
	struct device_node *syscon_np;
	struct regmap *regmap;

	if (property)
		syscon_np = of_parse_phandle(np, property, 0);
	else
		syscon_np = np;

	if (!syscon_np)
		return ERR_PTR(-ENODEV);

	regmap = syscon_node_to_regmap(syscon_np);

	return regmap;
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

	dev_dbg(dev, "map %pa-%pa registered\n", &res->start, &res->end);

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

core_platform_driver(syscon_driver);

MODULE_AUTHOR("Dong Aisheng <dong.aisheng@linaro.org>");
MODULE_DESCRIPTION("System Control driver");
MODULE_LICENSE("GPL v2");

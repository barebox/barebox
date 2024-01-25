// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2024 Marco Felsch, Pengutronix
/*
 * Based on Linux drivers/base/soc.c:
 * Copyright (C) ST-Ericsson SA 2011
 */

#include <common.h>
#include <init.h>
#include <of.h>

#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/err.h>

struct soc_device {
	struct device dev;
	struct soc_device_attribute *attr;
};

static struct bus_type soc_bus_type = {
	.name  = "soc",
};
static bool soc_bus_registered;

static void soc_device_add_params(struct soc_device *soc_dev)
{
	struct soc_device_attribute *attr = soc_dev->attr;
	struct device *dev = &soc_dev->dev;

	if (attr->machine)
		dev_add_param_string_fixed(dev, "machine", attr->machine);
	if (attr->family)
		dev_add_param_string_fixed(dev, "family", attr->family);
	if (attr->revision)
		dev_add_param_string_fixed(dev, "revision", attr->revision);
	if (attr->serial_number)
		dev_add_param_string_fixed(dev, "serial_number", attr->serial_number);
	if (attr->soc_id)
		dev_add_param_string_fixed(dev, "soc_id", attr->soc_id);
}

static void soc_device_get_machine(struct soc_device_attribute *soc_dev_attr)
{
	struct device_node *np;

	if (soc_dev_attr->machine)
		return;

	np = of_find_node_by_path("/");
	of_property_read_string(np, "model", &soc_dev_attr->machine);
	of_node_put(np);
}

static struct soc_device_attribute *early_soc_dev_attr;

struct soc_device *soc_device_register(struct soc_device_attribute *soc_dev_attr)
{
	struct soc_device *soc_dev;
	int ret;

	soc_device_get_machine(soc_dev_attr);

	if (!soc_bus_registered) {
		if (early_soc_dev_attr)
			return ERR_PTR(-EBUSY);
		early_soc_dev_attr = soc_dev_attr;
		return NULL;
	}

	soc_dev = kzalloc(sizeof(*soc_dev), GFP_KERNEL);
	if (!soc_dev) {
		ret = -ENOMEM;
		goto out1;
	}

	soc_dev->attr = soc_dev_attr;
	soc_dev->dev.bus = &soc_bus_type;
	soc_dev->dev.id = DEVICE_ID_DYNAMIC;

	dev_set_name(&soc_dev->dev, "soc");

	ret = device_register(&soc_dev->dev);
	if (ret) {
		put_device(&soc_dev->dev);
		goto out2;
	}

	soc_device_add_params(soc_dev);

	return soc_dev;

out2:
	kfree(soc_dev);
out1:
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(soc_device_register);

/* Ensure soc_dev->attr is freed after calling soc_device_unregister. */
void soc_device_unregister(struct soc_device *soc_dev)
{
	device_unregister(&soc_dev->dev);
	kfree(soc_dev);
	early_soc_dev_attr = NULL;
}
EXPORT_SYMBOL_GPL(soc_device_unregister);

static int __init soc_bus_register(void)
{
	int ret;

	ret = bus_register(&soc_bus_type);
	if (ret)
		return ret;
	soc_bus_registered = true;

	if (early_soc_dev_attr)
		return PTR_ERR(soc_device_register(early_soc_dev_attr));

	return 0;
}
core_initcall(soc_bus_register);

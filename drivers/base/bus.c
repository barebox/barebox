// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#include <common.h>
#include <linux/device/bus.h>
#include <driver.h>
#include <errno.h>
#include <of.h>

DEFINE_DEV_CLASS(bus_class, "bus");
EXPORT_SYMBOL(bus_class);

struct bus_type *get_bus_by_name(const char *name)
{
	struct bus_type *bus;

	for_each_bus(bus) {
		if(!strcmp(bus->name, name))
			return bus;
	}

	return NULL;
}
EXPORT_SYMBOL(get_bus_by_name);

int bus_register(struct bus_type *bus)
{
	int ret;

	if (get_bus_by_name(bus->name))
		return -EEXIST;

	dev_set_name(&bus->dev, "%s", bus->name);
	bus->dev.id = DEVICE_ID_SINGLE;

	ret = register_device(&bus->dev);
	if (ret)
		return ret;


	INIT_LIST_HEAD(&bus->device_list);
	INIT_LIST_HEAD(&bus->driver_list);

	class_add_device(&bus_class, &bus->dev);

	return 0;
}

int device_match(struct device *dev, const struct driver *drv)
{
	if (IS_ENABLED(CONFIG_OFDEVICE) && dev->of_node &&
	    drv->of_compatible)
		return of_match(dev, drv);

	if (drv->id_table) {
		const struct platform_device_id *id = drv->id_table;

		while (id->name) {
			if (!strcmp(id->name, dev->name)) {
				dev->id_entry = id;
				return true;
			}
			id++;
		}
	} else if (!strcmp(dev->name, drv->name)) {
		return true;
	}

	return false;
}

int device_match_of_modalias(struct device *dev, const struct driver *drv)
{
	const struct platform_device_id *id = drv->id_table;
	const char *of_modalias = NULL, *p;
	const struct property *prop;
	const char *compat;

	if (device_match(dev, drv))
		return true;

	if (!id || !IS_ENABLED(CONFIG_OFDEVICE) || !dev->of_node)
		return false;

	of_property_for_each_string(dev->of_node, "compatible", prop, compat) {
		p = strchr(compat, ',');
		of_modalias = p ? p + 1 : compat;

		for (id = drv->id_table; id->name; id++) {
			if (!strcmp(id->name, dev->name) || !strcmp(id->name, of_modalias)) {
				dev->id_entry = id;
				return true;
			}
		}
	}

	return false;
}

static struct device *__bus_for_each_dev(const struct bus_type *bus, struct device *start, void *data,
					 int (*fn)(struct device *dev, void *data), int *result)
{
	struct device *dev;
	int ret;

	bus_for_each_device(bus, dev) {
		if (start) {
			if (dev == start)
				start = NULL;
			continue;
		}

		ret = fn(dev, data);
		if (ret) {
			if (result)
				*result = ret;
			return dev;
		}
	}

	return NULL;
}

int bus_for_each_dev(const struct bus_type *bus, struct device *start, void *data,
		     int (*fn)(struct device *dev, void *data))
{
	int ret = 0;
	__bus_for_each_dev(bus, start, data, fn, &ret);
	return ret;
}

struct check_match_data {
	device_match_t match;
};

struct device *bus_find_device(const struct bus_type *bus, struct device *start,
			       const void *data, device_match_t match)
{
	return __bus_for_each_dev(bus, start, (void *)data,
				  (int (*)(struct device *dev, void *data))match,
				  NULL);
}

int device_match_name(struct device *dev, const void *name)
{
	return !strcmp(dev_name(dev), name);
}
EXPORT_SYMBOL_GPL(device_match_name);

int device_match_of_node(struct device *dev, const void *np)
{
	return np && dev->of_node == np;
}
EXPORT_SYMBOL_GPL(device_match_of_node);

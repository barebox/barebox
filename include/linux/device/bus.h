/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 */

#ifndef __BUS_H
#define __BUS_H

#include <device.h>

struct bus_type {
	char *name;
	int (*match)(struct device *dev, const struct driver *drv);
	int (*probe)(struct device *dev);
	void (*remove)(struct device *dev);

	struct device dev;

	struct list_head device_list;
	struct list_head driver_list;
};

int bus_register(struct bus_type *bus);
int device_match(struct device *dev, const struct driver *drv);
struct bus_type *get_bus_by_name(const char *name);

extern struct class bus_class;

/* Iterate over all buses
 */
#define for_each_bus(bus) class_for_each_container_of_device(&bus_class, bus, dev)

/* Iterate over all devices of a bus
 */
#define bus_for_each_device(bus, dev) list_for_each_entry(dev, &(bus)->device_list, bus_list)

/* Iterate over all drivers of a bus
 */
#define bus_for_each_driver(bus, drv) list_for_each_entry(drv, &(bus)->driver_list, bus_list)

int bus_for_each_dev(const struct bus_type *bus, struct device *start, void *data,
		     int (*fn)(struct device *dev, void *data));

/* Generic device matching functions that all busses can use to match with */
int device_match_name(struct device *dev, const void *name);
int device_match_of_node(struct device *dev, const void *np);

/* Matching function type for drivers/base APIs to find a specific device */
typedef int (*device_match_t)(struct device *dev, const void *data);

struct device *bus_find_device(const struct bus_type *bus, struct device *start,
			       const void *data, device_match_t match);
/**
 * bus_find_device_by_name - device iterator for locating a particular device
 * of a specific name.
 * @bus: bus type
 * @start: Device to begin with
 * @name: name of the device to match
 */
static inline struct device *bus_find_device_by_name(const struct bus_type *bus,
						     struct device *start,
						     const char *name)
{
	return bus_find_device(bus, start, name, device_match_name);
}

/**
 * bus_find_device_by_of_node : device iterator for locating a particular device
 * matching the of_node.
 * @bus: bus type
 * @np: of_node of the device to match.
 */
static inline struct device *
bus_find_device_by_of_node(const struct bus_type *bus, const struct device_node *np)
{
	return bus_find_device(bus, NULL, np, device_match_of_node);
}

extern struct bus_type platform_bus;

#endif

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

extern struct bus_type platform_bus;

#endif

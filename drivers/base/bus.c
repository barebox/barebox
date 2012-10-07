/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <driver.h>
#include <errno.h>

LIST_HEAD(bus_list);
EXPORT_SYMBOL(bus_list);

static struct bus_type *get_bus_by_name(const char *name)
{
	struct bus_type *bus;

	for_each_bus(bus) {
		if(!strcmp(bus->name, name))
			return bus;
	}

	return NULL;
}

int bus_register(struct bus_type *bus)
{
	if (get_bus_by_name(bus->name))
		return -EEXIST;

	INIT_LIST_HEAD(&bus->device_list);
	INIT_LIST_HEAD(&bus->driver_list);

	list_add_tail(&bus->list, &bus_list);

	return 0;
}

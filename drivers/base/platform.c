/*
 * bus.c - barebox driver model
 *
 * Copyright (c) 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <common.h>
#include <driver.h>

static int platform_match(struct device_d *dev, struct driver_d *drv)
{
	return strcmp(dev->name, drv->name) ? -1 : 0;
}

static int platform_probe(struct device_d *dev)
{
	return dev->driver->probe(dev);
}

static void platform_remove(struct device_d *dev)
{
	dev->driver->remove(dev);
}

struct bus_type platform_bus = {
	.name = "platform",
	.match = platform_match,
	.probe = platform_probe,
	.remove = platform_remove,
};

#if 0
LIST_HEAD(bus_list);
EXPORT_SYMBOL(bus_list);

int bus_register(struct bus_type *bus)
{
	list_add_tail(&bus->list, &bus_list);

	return 0;
}
#endif


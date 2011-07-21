/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>

struct device_d *add_generic_device(const char* devname, int id, const char *resname,
		resource_size_t start, resource_size_t size, unsigned int flags,
		void *pdata)
{
	struct device_d *dev;

	dev = xzalloc(sizeof(*dev));
	strcpy(dev->name, devname);
	dev->id = id;
	dev->resource = xzalloc(sizeof(struct resource));
	dev->num_resources = 1;
	if (resname)
		dev->resource[0].name = xstrdup(resname);
	dev->resource[0].start = start;
	dev->resource[0].size = size;
	dev->resource[0].flags = flags;
	dev->platform_data = pdata;

	register_device(dev);

	return dev;
}
EXPORT_SYMBOL(add_generic_device);

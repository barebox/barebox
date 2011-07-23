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

static struct device_d *alloc_device(const char* devname, int id, void *pdata)
{
	struct device_d *dev;

	dev = xzalloc(sizeof(*dev));
	strcpy(dev->name, devname);
	dev->id = id;
	dev->platform_data = pdata;

	return dev;
}

struct device_d *add_generic_device(const char* devname, int id, const char *resname,
		resource_size_t start, resource_size_t size, unsigned int flags,
		void *pdata)
{
	struct device_d *dev;

	dev = alloc_device(devname, id, pdata);
	dev->resource = xzalloc(sizeof(struct resource));
	dev->num_resources = 1;
	if (resname)
		dev->resource[0].name = xstrdup(resname);
	dev->resource[0].start = start;
	dev->resource[0].size = size;
	dev->resource[0].flags = flags;

	register_device(dev);

	return dev;
}
EXPORT_SYMBOL(add_generic_device);

#ifdef CONFIG_DRIVER_NET_DM9000
struct device_d *add_dm9000_device(int id, resource_size_t base,
		resource_size_t data, int flags, void *pdata)
{
	struct device_d *dev;
	resource_size_t size;

	dev = alloc_device("dm9000", id, pdata);
	dev->resource = xzalloc(sizeof(struct resource) * 2);
	dev->num_resources = 2;

	switch (flags) {
	case IORESOURCE_MEM_32BIT:
		size = 8;
		break;
	case IORESOURCE_MEM_16BIT:
		size = 4;
		break;
	case IORESOURCE_MEM_8BIT:
		size = 2;
		break;
	default:
		printf("dm9000: memory width flag missing\n");
		return NULL;
	}

	dev->resource[0].start = base;
	dev->resource[0].size = size;
	dev->resource[0].flags = IORESOURCE_MEM | flags;
	dev->resource[1].start = data;
	dev->resource[1].size = size;
	dev->resource[1].flags = IORESOURCE_MEM | flags;

	register_device(dev);

	return dev;
}
EXPORT_SYMBOL(add_dm9000_device);
#endif

#ifdef CONFIG_USB_EHCI
struct device_d *add_usb_ehci_device(int id, resource_size_t hccr,
		resource_size_t hcor, void *pdata)
{
	struct device_d *dev;

	dev = alloc_device("ehci", id, pdata);
	dev->resource = xzalloc(sizeof(struct resource) * 2);
	dev->num_resources = 2;
	dev->resource[0].start = hccr;
	dev->resource[0].flags = IORESOURCE_MEM;
	dev->resource[1].start = hcor;
	dev->resource[1].flags = IORESOURCE_MEM;

	register_device(dev);

	return dev;
}
EXPORT_SYMBOL(add_usb_ehci_device);
#endif

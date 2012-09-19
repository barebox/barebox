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
	struct resource *res;

	res = xzalloc(sizeof(struct resource));
	if (resname)
		res[0].name = xstrdup(resname);
	res[0].start = start;
	res[0].end = start + size - 1;
	res[0].flags = flags;

	return add_generic_device_res(devname, id, res, 1, pdata);
}
EXPORT_SYMBOL(add_generic_device);

struct device_d *add_generic_device_res(const char* devname, int id,
		struct resource *res, int nb, void *pdata)
{
	struct device_d *dev;

	dev = alloc_device(devname, id, pdata);
	dev->resource = res;
	dev->num_resources = nb;

	platform_device_register(dev);

	return dev;
}
EXPORT_SYMBOL(add_generic_device_res);

#ifdef CONFIG_DRIVER_NET_DM9K
struct device_d *add_dm9000_device(int id, resource_size_t base,
		resource_size_t data, int flags, void *pdata)
{
	struct resource *res;
	resource_size_t size;

	res = xzalloc(sizeof(struct resource) * 2);

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

	res[0].start = base;
	res[0].end = base + size - 1;
	res[0].flags = IORESOURCE_MEM | flags;
	res[1].start = data;
	res[1].end = data + size - 1;
	res[1].flags = IORESOURCE_MEM | flags;

	return add_generic_device_res("dm9000", id, res, 2, pdata);
}
EXPORT_SYMBOL(add_dm9000_device);
#endif

#ifdef CONFIG_USB_EHCI
struct device_d *add_usb_ehci_device(int id, resource_size_t hccr,
		resource_size_t hcor, void *pdata)
{
	struct resource *res;

	res = xzalloc(sizeof(struct resource) * 2);
	res[0].start = hccr;
	res[0].end = hccr + 0x10 - 1;
	res[0].flags = IORESOURCE_MEM;
	res[1].start = hcor;
	res[1].end = hcor + 0xc0 - 1;
	res[1].flags = IORESOURCE_MEM;

	return add_generic_device_res("ehci", id, res, 2, pdata);
}
EXPORT_SYMBOL(add_usb_ehci_device);
#endif

#ifdef CONFIG_DRIVER_NET_KS8851_MLL
struct device_d *add_ks8851_device(int id, resource_size_t addr,
		resource_size_t addr_cmd, int flags, void *pdata)
{
	struct resource *res;
	resource_size_t size;

	switch (flags) {
	case IORESOURCE_MEM_16BIT:
		size = 2;
		break;
	case IORESOURCE_MEM_8BIT:
		size = 1;
		break;
	default:
		printf("ks8851: memory width flag missing\n");
		return NULL;
	}

	res = xzalloc(sizeof(struct resource) * 2);

	res[0].start = addr;
	res[0].end = addr + size - 1;
	res[0].flags = IORESOURCE_MEM | flags;
	res[1].start = addr_cmd;
	res[1].end = addr_cmd + size - 1;
	res[1].flags = IORESOURCE_MEM | flags;

	return add_generic_device_res("ks8851_mll", id, res, 2, pdata);
}
EXPORT_SYMBOL(add_ks8851_device);
#endif

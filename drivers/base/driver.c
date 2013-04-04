/*
 * driver.c - barebox driver model
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 */

/**
 * @file
 * @brief barebox's driver model, and devinfo command
 */

#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <errno.h>
#include <fs.h>
#include <linux/list.h>
#include <complete.h>

LIST_HEAD(device_list);
EXPORT_SYMBOL(device_list);

LIST_HEAD(driver_list);
EXPORT_SYMBOL(driver_list);

static LIST_HEAD(active);

struct device_d *get_device_by_name(const char *name)
{
	struct device_d *dev;

	for_each_device(dev) {
		if(!strcmp(dev_name(dev), name))
			return dev;
	}

	return NULL;
}

static struct device_d *get_device_by_name_id(const char *name, int id)
{
	struct device_d *dev;

	for_each_device(dev) {
		if(!strcmp(dev->name, name) && id == dev->id)
			return dev;
	}

	return NULL;
}

int get_free_deviceid(const char *name_template)
{
	int i = 0;

	while (1) {
		if (!get_device_by_name_id(name_template, i))
			return i;
		i++;
	};
}

int device_probe(struct device_d *dev)
{
	int ret;

	ret = dev->bus->probe(dev);
	if (ret) {
		dev_err(dev, "probe failed: %s\n", strerror(-ret));
		return ret;
	}

	list_add(&dev->active, &active);

	return 0;
}

static int match(struct driver_d *drv, struct device_d *dev)
{
	int ret;

	if (dev->driver)
		return -1;

	dev->driver = drv;

	if (dev->bus->match(dev, drv))
		goto err_out;
	ret = device_probe(dev);
	if (ret)
		goto err_out;

	return 0;
err_out:
	dev->driver = NULL;
	return -1;
}

int register_device(struct device_d *new_device)
{
	struct driver_d *drv;

	if (new_device->id == DEVICE_ID_DYNAMIC) {
		new_device->id = get_free_deviceid(new_device->name);
	} else {
		if (get_device_by_name_id(new_device->name, new_device->id)) {
			eprintf("register_device: already registered %s\n",
				dev_name(new_device));
			return -EINVAL;
		}
	}

	debug ("register_device: %s\n", dev_name(new_device));

	list_add_tail(&new_device->list, &device_list);
	INIT_LIST_HEAD(&new_device->children);
	INIT_LIST_HEAD(&new_device->cdevs);
	INIT_LIST_HEAD(&new_device->parameters);
	INIT_LIST_HEAD(&new_device->active);

	if (new_device->bus) {
		if (!new_device->parent)
			new_device->parent = new_device->bus->dev;

		list_add_tail(&new_device->bus_list, &new_device->bus->device_list);

		bus_for_each_driver(new_device->bus, drv) {
			if (!match(drv, new_device))
				break;
		}
	}

	if (new_device->parent)
		list_add_tail(&new_device->sibling, &new_device->parent->children);

	return 0;
}
EXPORT_SYMBOL(register_device);

int unregister_device(struct device_d *old_dev)
{
	struct cdev *cdev, *ct;
	struct device_d *child, *dt;

	dev_dbg(old_dev, "unregister\n");

	if (old_dev->driver)
		old_dev->bus->remove(old_dev);

	list_for_each_entry_safe(child, dt, &old_dev->children, sibling) {
		dev_dbg(old_dev, "unregister child %s\n", dev_name(child));
		unregister_device(child);
	}

	list_for_each_entry_safe(cdev, ct, &old_dev->cdevs, devices_list) {
		if (cdev->flags & DEVFS_IS_PARTITION) {
			dev_dbg(old_dev, "unregister part %s\n", cdev->name);
			devfs_del_partition(cdev->name);
		}
	}

	list_del(&old_dev->list);
	list_del(&old_dev->bus_list);
	list_del(&old_dev->active);

	/* remove device from parents child list */
	if (old_dev->parent)
		list_del(&old_dev->sibling);

	return 0;
}
EXPORT_SYMBOL(unregister_device);

struct driver_d *get_driver_by_name(const char *name)
{
	struct driver_d *drv;

	for_each_driver(drv) {
		if(!strcmp(name, drv->name))
			return drv;
	}

	return NULL;
}

static void noinfo(struct device_d *dev)
{
	printf("no info available for %s\n", dev_name(dev));
}

static void noshortinfo(struct device_d *dev)
{
}

int register_driver(struct driver_d *drv)
{
	struct device_d *dev = NULL;

	debug("register_driver: %s\n", drv->name);

	BUG_ON(!drv->bus);

	list_add_tail(&drv->list, &driver_list);
	list_add_tail(&drv->bus_list, &drv->bus->driver_list);

	if (!drv->info)
		drv->info = noinfo;
	if (!drv->shortinfo)
		drv->shortinfo = noshortinfo;

	bus_for_each_device(drv->bus, dev)
		match(drv, dev);

	return 0;
}
EXPORT_SYMBOL(register_driver);

struct resource *dev_get_resource(struct device_d *dev, int num)
{
	int i, n = 0;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *res = &dev->resource[i];
		if (resource_type(res) == IORESOURCE_MEM) {
			if (n == num)
				return res;
			n++;
		}
	}

	return NULL;
}

void *dev_get_mem_region(struct device_d *dev, int num)
{
	struct resource *res;

	res = dev_get_resource(dev, num);
	if (!res)
		return NULL;

	return (void __force *)res->start;
}
EXPORT_SYMBOL(dev_get_mem_region);

struct resource *dev_get_resource_by_name(struct device_d *dev,
					  const char *name)
{
	int i;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *res = &dev->resource[i];
		if (resource_type(res) != IORESOURCE_MEM)
			continue;
		if (!res->name)
			continue;
		if (!strcmp(name, res->name))
			return res;
	}

	return NULL;
}

void *dev_get_mem_region_by_name(struct device_d *dev, const char *name)
{
	struct resource *res;

	res = dev_get_resource_by_name(dev, name);
	if (!res)
		return NULL;

	return (void __force *)res->start;
}
EXPORT_SYMBOL(dev_get_mem_region_by_name);

void __iomem *dev_request_mem_region_by_name(struct device_d *dev, const char *name)
{
	struct resource *res;

	res = dev_get_resource_by_name(dev, name);
	if (!res)
		return NULL;

	res = request_iomem_region(dev_name(dev), res->start, res->end);
	if (!res)
		return NULL;

	return (void __force __iomem *)res->start;
}
EXPORT_SYMBOL(dev_request_mem_region_by_name);

void __iomem *dev_request_mem_region(struct device_d *dev, int num)
{
	struct resource *res;

	res = dev_get_resource(dev, num);
	if (!res)
		return NULL;

	res = request_iomem_region(dev_name(dev), res->start, res->end);
	if (!res)
		return NULL;

	return (void __force __iomem *)res->start;
}
EXPORT_SYMBOL(dev_request_mem_region);

int dev_protect(struct device_d *dev, size_t count, unsigned long offset, int prot)
{
	printf("%s: currently broken\n", __func__);
	return -EINVAL;
}

int generic_memmap_ro(struct cdev *cdev, void **map, int flags)
{
	if (!cdev->dev)
		return -EINVAL;

	if (flags & PROT_WRITE)
		return -EACCES;
	*map = dev_get_mem_region(cdev->dev, 0);
	return 0;
}

int generic_memmap_rw(struct cdev *cdev, void **map, int flags)
{
	if (!cdev->dev)
		return -EINVAL;

	*map = dev_get_mem_region(cdev->dev, 0);
	return 0;
}

int dummy_probe(struct device_d *dev)
{
	return 0;
}
EXPORT_SYMBOL(dummy_probe);

const char *dev_id(const struct device_d *dev)
{
	static char buf[MAX_DRIVER_NAME + 16];

	if (dev->id != DEVICE_ID_SINGLE)
		snprintf(buf, sizeof(buf), FORMAT_DRIVER_NAME_ID, dev->name, dev->id);
	else
		snprintf(buf, sizeof(buf), "%s", dev->name);

	return buf;
}

int dev_printf(const struct device_d *dev, const char *format, ...)
{
	va_list args;
	int ret = 0;

	if (dev->driver && dev->driver->name)
		ret += printf("%s ", dev->driver->name);

	ret += printf("%s: ", dev_name(dev));

	va_start(args, format);

	ret += vprintf(format, args);

	va_end(args);

	return ret;
}

void devices_shutdown(void)
{
	struct device_d *dev;

	list_for_each_entry(dev, &active, active) {
		if (dev->driver->remove)
			dev->driver->remove(dev);
	}
}

int dev_get_drvdata(struct device_d *dev, unsigned long *data)
{
	if (dev->of_id_entry) {
		*data = dev->of_id_entry->data;
		return 0;
	}

	if (dev->id_entry) {
		*data = dev->id_entry->driver_data;
		return 0;
	}

	return -ENODEV;
}

#ifdef CONFIG_CMD_DEVINFO
static int do_devinfo_subtree(struct device_d *dev, int depth)
{
	struct device_d *child;
	struct cdev *cdev;
	int i;

	for (i = 0; i < depth; i++)
		printf("     ");

	printf("`---- %s", dev_name(dev));
	if (!list_empty(&dev->cdevs)) {
		printf("\n");
		list_for_each_entry(cdev, &dev->cdevs, devices_list) {
			for (i = 0; i < depth + 1; i++)
				printf("     ");
			printf("`---- 0x%08llx-0x%08llx: /dev/%s\n",
					cdev->offset,
					cdev->offset + cdev->size - 1,
					cdev->name);
		}
	} else {
		printf("\n");
	}

	if (!list_empty(&dev->children)) {
		device_for_each_child(dev, child) {
			do_devinfo_subtree(child, depth + 1);
		}
	}

	return 0;
}

static int do_devinfo(int argc, char *argv[])
{
	struct device_d *dev;
	struct driver_d *drv;
	struct param_d *param;
	int i;
	struct resource *res;

	if (argc == 1) {
		printf("devices:\n");

		for_each_device(dev) {
			if (!dev->parent)
				do_devinfo_subtree(dev, 0);
		}

		printf("\ndrivers:\n");
		for_each_driver(drv)
			printf("%s\n",drv->name);
	} else {
		dev = get_device_by_name(argv[1]);

		if (!dev) {
			printf("no such device: %s\n",argv[1]);
			return -1;
		}

		printf("resources:\n");
		for (i = 0; i < dev->num_resources; i++) {
			res = &dev->resource[i];
			printf("num   : %d\n", i);
			if (res->name)
				printf("name  : %s\n", res->name);
			printf("start : " PRINTF_CONVERSION_RESOURCE "\nsize  : "
					PRINTF_CONVERSION_RESOURCE "\n",
			       res->start, resource_size(res));
		}

		printf("driver: %s\n", dev->driver ?
				dev->driver->name : "none");

		printf("bus: %s\n\n", dev->bus ?
				dev->bus->name : "none");

		if (dev->driver)
			dev->driver->info(dev);

		printf("%s\n", list_empty(&dev->parameters) ?
				"no parameters available" : "Parameters:");

		list_for_each_entry(param, &dev->parameters, list)
			printf("%16s = %s\n", param->name, dev_get_param(dev, param->name));
#ifdef CONFIG_OFDEVICE
		if (dev->device_node) {
			printf("\ndevice node: %s\n", dev->device_node->full_name);
			of_print_nodes(dev->device_node, 0);
		}
#endif
	}

	return 0;
}

BAREBOX_CMD_HELP_START(devinfo)
BAREBOX_CMD_HELP_USAGE("devinfo [DEVICE]\n")
BAREBOX_CMD_HELP_SHORT("Output device information.\n")
BAREBOX_CMD_HELP_END

/**
 * @page devinfo_command

If called without arguments, devinfo shows a summary of the known
devices and drivers.

If called with a device path being the argument, devinfo shows more
default information about this device and its parameters.

Example from an MPC5200 based system:

@verbatim
  barebox:/ devinfo /dev/eth0
  base  : 0x1002b000
  size  : 0x00000000
  driver: fec_mpc5xxx

  no info available for eth0
  Parameters:
      ipaddr = 192.168.23.197
     ethaddr = 80:81:82:83:84:86
     gateway = 192.168.23.1
     netmask = 255.255.255.0
    serverip = 192.168.23.2
@endverbatim
 */

BAREBOX_CMD_START(devinfo)
	.cmd		= do_devinfo,
	.usage		= "Show information about devices and drivers.",
	BAREBOX_CMD_HELP(cmd_devinfo_help)
	BAREBOX_CMD_COMPLETE(device_complete)
BAREBOX_CMD_END
#endif


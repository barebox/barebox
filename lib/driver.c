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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

LIST_HEAD(device_list);
EXPORT_SYMBOL(device_list);

LIST_HEAD(driver_list);
EXPORT_SYMBOL(driver_list);

static LIST_HEAD(active);

struct device_d *get_device_by_name(const char *name)
{
	struct device_d *dev;
	char devname[MAX_DRIVER_NAME + 3];

	for_each_device(dev) {
		sprintf(devname, "%s%d", dev->name, dev->id);
		if(!strcmp(name, devname))
			return dev;
	}

	return NULL;
}

int get_free_deviceid(const char *name_template)
{
	int i = 0;
	char name[MAX_DRIVER_NAME + 3];

	while (1) {
		sprintf(name, "%s%d", name_template, i);
		if (!get_device_by_name(name))
			return i;
		i++;
	};
}

static int match(struct driver_d *drv, struct device_d *dev)
{
	if (dev->driver)
		return -1;

	dev->driver = drv;

	if (dev->bus != drv->bus)
		goto err_out;
	if (dev->bus->match(dev, drv))
		goto err_out;
	if (dev->bus->probe(dev))
		goto err_out;

	list_add(&dev->active, &active);

	return 0;
err_out:
	dev->driver = NULL;
	return -1;
}

int register_device(struct device_d *new_device)
{
	struct driver_d *drv;

	new_device->id = get_free_deviceid(new_device->name);

	debug ("register_device: %s\n",new_device->name);

	if (!new_device->bus) {
//		dev_err(new_device, "no bus type associated. Needs fixup\n");
		new_device->bus = &platform_bus;
	}

	list_add_tail(&new_device->list, &device_list);
	INIT_LIST_HEAD(&new_device->children);
	INIT_LIST_HEAD(&new_device->cdevs);

	for_each_driver(drv) {
		if (!match(drv, new_device))
			break;
	}

	return 0;
}
EXPORT_SYMBOL(register_device);

int unregister_device(struct device_d *old_dev)
{
	debug("unregister_device: %s:%s\n",old_dev->name, old_dev->id);

	if (!list_empty(&old_dev->children)) {
		errno = -EBUSY;
		return errno;
	}

	if (old_dev->driver)
		old_dev->bus->remove(old_dev);

	list_del(&old_dev->list);

	/* remove device from parents child list */
	if (old_dev->parent)
		list_del(&old_dev->sibling);

	return 0;
}
EXPORT_SYMBOL(unregister_device);

int dev_add_child(struct device_d *dev, struct device_d *child)
{
	child->parent = dev;

	list_add_tail(&child->sibling, &dev->children);

	return 0;
}
EXPORT_SYMBOL(dev_add_child);

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
	printf("no info available for %s\n", dev->name);
}

static void noshortinfo(struct device_d *dev)
{
}

int register_driver(struct driver_d *drv)
{
	struct device_d *dev = NULL;

	debug("register_driver: %s\n", drv->name);

	if (!drv->bus) {
//		pr_err("driver %s has no bus type associated. Needs fixup\n", drv->name);
		drv->bus = &platform_bus;
	}

	list_add_tail(&drv->list, &driver_list);

	if (!drv->info)
		drv->info = noinfo;
	if (!drv->shortinfo)
		drv->shortinfo = noshortinfo;

	for_each_device(dev)
		match(drv, dev);

	return 0;
}
EXPORT_SYMBOL(register_driver);

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
	*map = (void *)cdev->dev->map_base;
	return 0;
}

int generic_memmap_rw(struct cdev *cdev, void **map, int flags)
{
	if (!cdev->dev)
		return -EINVAL;

	*map = (void *)cdev->dev->map_base;
	return 0;
}

int dummy_probe(struct device_d *dev)
{
	return 0;
}
EXPORT_SYMBOL(dummy_probe);

static int do_devinfo_subtree(struct device_d *dev, int depth, char edge)
{
	struct device_d *child;
	struct cdev *cdev;
	int i;

	for (i = 0; i < depth; i++)
		printf("|    ");

	printf("%c----%s%d", edge, dev->name, dev->id);
	if (!list_empty(&dev->cdevs)) {
		printf(" (");
		list_for_each_entry(cdev, &dev->cdevs, devices_list) {
			printf("%s", cdev->name);
			if (!list_is_last(&cdev->devices_list, &dev->cdevs))
				printf(", ");
		}
		printf(")");
	}
	printf("\n");

	if (!list_empty(&dev->children)) {
		device_for_each_child(dev, child) {
			do_devinfo_subtree(child, depth + 1,
					list_is_last(&child->sibling,
						&dev->children) ? '`' : '|');
		}
	}

	return 0;
}

const char *dev_id(const struct device_d *dev)
{
	static char buf[sizeof(unsigned long) * 2];

	sprintf(buf, "%s%d", dev->name, dev->id);

	return buf;
}

void devices_shutdown(void)
{
	struct device_d *dev;

	list_for_each_entry(dev, &active, active) {
		if (dev->driver->remove)
			dev->driver->remove(dev);
	}
}

#ifdef CONFIG_CMD_DEVINFO

static int do_devinfo ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	struct device_d *dev;
	struct driver_d *drv;
	struct param_d *param;

	if (argc == 1) {
		printf("devices:\n");

		for_each_device(dev) {
			if (!dev->parent)
				do_devinfo_subtree(dev, 0, '|');
		}

		printf("\ndrivers:\n");
		for_each_driver(drv)
			printf("%10s\n",drv->name);
	} else {
		struct device_d *dev = get_device_by_name(argv[1]);

		if (!dev) {
			printf("no such device: %s\n",argv[1]);
			return -1;
		}

		printf("base  : 0x%08x\nsize  : 0x%08x\ndriver: %s\n\n",
			dev->map_base, dev->size,
			dev->driver ? 
				dev->driver->name : "none");

		if (dev->driver)
			dev->driver->info(dev);

		param = dev->param;

		printf("%s\n", param ?
				"Parameters:" : "no parameters available");

		while (param) {
			printf("%16s = %s\n", param->name, param->value);
			param = param->next;
		}

	}

	return 0;
}

static const __maybe_unused char cmd_devinfo_help[] =
"Usage: devinfo [DEVICE]\n"
"If called without arguments devinfo shows a summary about known devices and\n"
"drivers. If called with a device path as argument devinfo shows more detailed\n"
"information about this device and its parameters.\n";

BAREBOX_CMD_START(devinfo)
	.cmd		= do_devinfo,
	.usage		= "display info about devices and drivers",
	BAREBOX_CMD_HELP(cmd_devinfo_help)
BAREBOX_CMD_END

#endif

/**
 * @page devinfo_command devinfo
 *
 * Usage is: devinfo /dev/\<device>
 *
 * If called without arguments devinfo shows a summary about known devices and
 * drivers. If called with a device path as argument devinfo shows more
 * detailed information about this device and its parameters.
 *
 * Example from an MPC5200 based system:
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
 *
 */

/*
 * driver.c - U-Boot driver model
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

#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <errno.h>
#include <fs.h>
#include <list.h>

LIST_HEAD(device_list);
EXPORT_SYMBOL(device_list);

LIST_HEAD(driver_list);
EXPORT_SYMBOL(driver_list);

struct device_d *get_device_by_id(const char *id)
{
	struct device_d *dev;

	for_each_device(dev) {
		if(!strcmp(id, dev->id))
			return dev;
	}

	return NULL;
}

int get_free_deviceid(char *id, const char *id_template)
{
	int i = 0;

	while (1) {
		sprintf(id, "%s%d", id_template, i);
		if (!get_device_by_id(id))
			return 0;
		i++;
	};

	return -1;
}

static int match(struct driver_d *drv, struct device_d *dev)
{
	if (strcmp(dev->name, drv->name))
		return -1;
	if (dev->type != drv->type)
		return -1;
	if(drv->probe(dev))
		return -1;

	dev->driver = drv;

	return 0;
}

int register_device(struct device_d *new_device)
{
	struct driver_d *drv;

	if(*new_device->id && get_device_by_id(new_device->id)) {
		printf("device %s already exists\n", new_device->id);
		return -EINVAL;
	}
	debug ("register_device: %s\n",new_device->name);

	list_add_tail(&new_device->list, &device_list);
	INIT_LIST_HEAD(&new_device->children);

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
		old_dev->driver->remove(old_dev);

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
	printf("no info available for %s\n", dev->id);
}

static void noshortinfo(struct device_d *dev)
{
}

int register_driver(struct driver_d *drv)
{
	struct device_d *dev = NULL;

	debug("register_driver: %s\n", drv->name);

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

/* Get a device struct from the beginning of the string. Default to mem if no
 * device is given, return NULL if a unknown device is given.
 * If endp is not NULL, this function stores a pointer to the first character
 * after the device name in *endp.
 */
struct device_d *get_device_by_path(const char *path)
{
	if (strncmp(path, "/dev/", 5))
		return NULL;

	return get_device_by_id(path + 5);
}

ssize_t dev_read(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
	if (dev->driver->read)
		return dev->driver->read(dev, buf, count, offset, flags);
	errno = -ENOSYS;
	return -ENOSYS;
}

ssize_t dev_write(struct device_d *dev, const void *buf, size_t count, unsigned long offset, ulong flags)
{
	if (dev->driver->write)
		return dev->driver->write(dev, buf, count, offset, flags);
	errno = -ENOSYS;
	return -ENOSYS;
}

int dev_erase(struct device_d *dev, size_t count, unsigned long offset)
{
	if (dev->driver->erase)
		return dev->driver->erase(dev, count, offset);
	errno = -ENOSYS;
	return -ENOSYS;
}

int dev_protect(struct device_d *dev, size_t count, unsigned long offset, int prot)
{
	if (dev->driver->protect)
		return dev->driver->protect(dev, count, offset, prot);
	errno = -ENOSYS;
	return -ENOSYS;
}

int dev_memmap(struct device_d *dev, void **map, int flags)
{
	if (dev->driver->memmap)
		return dev->driver->memmap(dev, map, flags);
	errno = -ENOSYS;
	return -ENOSYS;
}

int generic_memmap_ro(struct device_d *dev, void **map, int flags)
{
	if (flags & PROT_WRITE)
		return -EACCES;
	*map = (void *)dev->map_base;
	return 0;
}

int generic_memmap_rw(struct device_d *dev, void **map, int flags)
{
	*map = (void *)dev->map_base;
	return 0;
}

int dummy_probe(struct device_d *dev)
{
	return 0;
}

static int do_devinfo_subtree(struct device_d *dev, int depth, char edge)
{
	struct device_d *child;
	int i;

	for (i = 0; i < depth; i++)
		printf("|    ");

	if (*dev->id)
		printf("%c----%s\n", edge, dev->id);
	else if (dev->type == DEVICE_TYPE_FS)
		printf("%c----filesystem: %s\n", edge,
				fsdev_get_mountpoint((struct fs_device_d *)
					dev->type_data));

	if (!list_empty(&dev->children)) {
		device_for_each_child(dev, child) {
			do_devinfo_subtree(child, depth + 1,
					list_is_last(&child->sibling,
						&dev->children) ? '`' : '|');
		}
	}

	return 0;
}

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
		struct device_d *dev = get_device_by_path(argv[1]);

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

static __maybe_unused char cmd_devinfo_help[] =
"Usage: devinfo [DEVICE]\n"
"If called without arguments devinfo shows a summary about known devices and\n"
"drivers. If called with a device path as argument devinfo shows more detailed\n"
"informations about this device and its parameters.\n";

U_BOOT_CMD_START(devinfo)
	.maxargs	= 2,
	.cmd		= do_devinfo,
	.usage		= "display info about devices and drivers",
	U_BOOT_CMD_HELP(cmd_devinfo_help)
U_BOOT_CMD_END

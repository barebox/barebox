/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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

/**
 * @file
 * @brief Partition handling on top of devices
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <partition.h>
#include <xfuncs.h>

struct device_d *dev_add_partition(struct device_d *dev, unsigned long offset,
		size_t size, int flags, const char *name)
{
	struct partition *part;

	if (offset + size > dev->size)
		return NULL;

	part = xzalloc(sizeof(struct partition));

	strcpy(part->device.name, "partition");
	part->device.map_base = dev->map_base + offset;
	part->device.size = size;
	part->device.type_data = part;
	get_free_deviceid(part->device.id, name);

	part->offset = offset;
	part->physdev = dev;
	part->flags = flags;

	register_device(&part->device);
	dev_add_child(dev, &part->device);

	if (part->device.driver)
		return &part->device;

	free(part);
	return 0;
}

static int part_erase(struct device_d *dev, size_t count, unsigned long offset)
{
	struct partition *part = dev->type_data;

	if (part->physdev->driver->erase)
		return part->physdev->driver->erase(part->physdev, count, offset + part->offset);

	return -1;
}

static int part_protect(struct device_d *dev, size_t count, unsigned long offset, int prot)
{
	struct partition *part = dev->type_data;

	if (part->physdev->driver->protect)
		return part->physdev->driver->protect(part->physdev, count, offset + part->offset, prot);

	return -1;
}

static int part_memmap(struct device_d *dev, void **map, int flags)
{
	struct partition *part = dev->type_data;
	int ret;

	if (part->physdev->driver->memmap) {
		ret = part->physdev->driver->memmap(part->physdev, map, flags);
		if (ret)
			return ret;
		*map = (void *)((unsigned long)*map + part->offset);
		return 0;
	}

	return -1;
}

static ssize_t part_read(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
	struct partition *part = dev->type_data;

	return dev_read(part->physdev, buf, count, offset + part->offset, flags);
}

static ssize_t part_write(struct device_d *dev, const void *buf, size_t count, unsigned long offset, ulong flags)
{
	struct partition *part = dev->type_data;

	if (part->flags & PARTITION_READONLY)
 		return -EROFS;
	else
		return dev_write(part->physdev, buf, count, offset + part->offset, flags);
}

static int part_probe(struct device_d *dev)
{
#ifdef DEBUG
	struct partition *part = dev->type_data;
#endif

	debug("registering partition %s on device %s (size=0x%08x, name=%s)\n",
			dev->id, part->physdev->id, dev->size, part->name);
	return 0;
}

static int part_remove(struct device_d *dev)
{
	return 0;
}

struct driver_d part_driver = {
	.name  	= "partition",
	.probe 	= part_probe,
	.remove = part_remove,
	.read  	= part_read,
	.write 	= part_write,
	.erase 	= part_erase,
	.protect= part_protect,
	.memmap = part_memmap,
};

static int partition_init(void)
{
	return register_driver(&part_driver);
}

device_initcall(partition_init);

/**
@page partitions Partition Handling

Partitions are runtime informartion only, not permanent. So they must be set
everytime the system started. The required command can be embedded into the
default environment.

@note Partitions defined in this way are intended to be used with the kernel
command line partition parsing feature. In Uboot2 these types of partitions are
handled in the same way as every other device.

@par The addpart command

What we want:

@code
 device nor0
    |--- partition 0
    |--- partition 1
    |--- partition 2
    |--- partition 3
    `--- partition 4
@endcode

How to get:

@code
$ addpart /dev/nor0 256k(uboot),128k(env),256k(bla),1024k(blubb),2048k(friesel)
$ devinfo
 |---nor0.uboot
 |---nor0.env
 |---nor0.bla
 |---nor0.blubb
 |---nor0.friesel
@endcode

@par Partitions with sub partitions:

Partitions are based on devices. And partitions will result into devices. So
there is a way to create sub partitions on partitions.

What we want:

@code
 device nor0
    |--- partition 0
    |--- partition 1
    |--- partition 2
    |--- partition 3
    `--- partition 4
           |--- partition 0
           `--- partition 1
@endcode

How to get:

@code
$ addpart /dev/nor0 256k(uboot),128k(env),256k(bla),1M(blubb),2048k(friesel)
$ devinfo
 |---nor0.uboot
 |---nor0.env
 |---nor0.bla
 |---nor0.blubb
 |---nor0.friesel

$ addpart /dev/nor0.friesel 1024(fussel),1024k(boerks)
$ devinfo
 |---nor0.uboot
 |---nor0.env
 |---nor0.bla
 |---nor0.blubb
 |---nor0.friesel
        |---nor0.friesel.fussel
        `---nor0.friesel.boerks
@endcode

@par Partitions forwarding to the kernel:

@code
$ device="nor0"
$ partitions="256k(uboot),128k(env),256k(bla),1024k(blubb),2048k(friesel)"
$ addpart /dev/$device:$partitions
@endcode

@par Removing partitions:

As partitions are a logically information only, they can be removed from a
device at runtime. You can't remove a single partition within others on the
same device. Only all partitions on the given device can be removed with this
command.

As sub partitions occure as devices you also can remove sub partitions from
their parent in this way. If the parent itself will be removed all its sub
partitions will also removed.

*/
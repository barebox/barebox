// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <init.h>

static struct cdev_operations memops = {
	.read  = mem_read,
	.write = mem_write,
	.memmap = generic_memmap_rw,
};

static int mem_probe(struct device_d *dev)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));
	dev->priv = cdev;

	cdev->name = (char*)dev->resource[0].name;
	cdev->size = min_t(unsigned long long, resource_size(&dev->resource[0]),
			   S64_MAX);
	cdev->ops = &memops;
	cdev->dev = dev;

	devfs_create(cdev);

	return 0;
}

static struct driver_d mem_drv = {
	.name  = "mem",
	.probe = mem_probe,
};

static int mem_init(void)
{
	add_mem_device("mem", 0, ~0, IORESOURCE_MEM_WRITEABLE);
	return platform_driver_register(&mem_drv);
}
device_initcall(mem_init);

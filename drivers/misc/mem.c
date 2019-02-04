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
	if (dev->resource[0].start == 0 && dev->resource[0].end == ~0) {
		/*
		 * Special case for /dev/mem. We can't express it's size as it's
		 * outside of our address range. Set DEVFS_IS_CHARACTER_DEV to
		 * bypass size checks.
		 */
		cdev->size = 0;
		cdev->flags = DEVFS_IS_CHARACTER_DEV;
	} else {
		cdev->size = resource_size(&dev->resource[0]);
	}

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
	struct device_d *dev;
	struct resource res = {
		.start = 0,
		.end = ~0,
		.flags = IORESOURCE_MEM,
		.name = "mem",
	};
	int ret;

	dev = device_alloc("mem", DEVICE_ID_DYNAMIC);
	if (!dev)
		return -ENOMEM;

	dev->resource = xmemdup(&res, sizeof(res));
	dev->num_resources = 1;

	ret = platform_device_register(dev);
	if (ret)
		return ret;

	return platform_driver_register(&mem_drv);
}
device_initcall(mem_init);

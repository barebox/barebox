/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

#ifdef	CMD_MEM_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

static struct cdev_operations memops = {
	.read  = mem_read,
	.write = mem_write,
	.memmap = generic_memmap_rw,
	.lseek = dev_lseek_default,
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

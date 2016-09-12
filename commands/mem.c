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

char *mem_rw_buf;

/*
 * Common function for parsing options for the 'md', 'mw', 'memcpy', 'memcmp'
 * commands.
 */
int mem_parse_options(int argc, char *argv[], char *optstr, int *mode,
		char **sourcefile, char **destfile, int *swab)
{
	int opt;

	while((opt = getopt(argc, argv, optstr)) > 0) {
		switch(opt) {
		case 'b':
			*mode = O_RWSIZE_1;
			break;
		case 'w':
			*mode = O_RWSIZE_2;
			break;
		case 'l':
			*mode = O_RWSIZE_4;
			break;
		case 'q':
			*mode = O_RWSIZE_8;
			break;
		case 's':
			*sourcefile = optarg;
			break;
		case 'd':
			*destfile = optarg;
			break;
		case 'x':
			*swab = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	return 0;
}

static struct file_operations memops = {
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
	cdev->size = (unsigned long)resource_size(&dev->resource[0]);
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
	mem_rw_buf = malloc(RW_BUF_SIZE);
	if(!mem_rw_buf)
		return -ENOMEM;

	add_mem_device("mem", 0, ~0, IORESOURCE_MEM_WRITEABLE);
	return platform_driver_register(&mem_drv);
}
device_initcall(mem_init);

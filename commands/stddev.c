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

#include <common.h>
#include <init.h>
#include <stdlib.h>

static ssize_t zero_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	memset(buf, 0, count);
	return count;
}

static struct file_operations zeroops = {
	.read  = zero_read,
	.lseek = dev_lseek_default,
};

static int zero_init(void)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));

	cdev->name = "zero";
	cdev->flags = DEVFS_IS_CHARACTER_DEV;
	cdev->ops = &zeroops;

	devfs_create(cdev);

	return 0;
}

device_initcall(zero_init);

static ssize_t full_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	memset(buf, 0xff, count);
	return count;
}

static struct file_operations fullops = {
	.read  = full_read,
	.lseek = dev_lseek_default,
};

static int full_init(void)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));

	cdev->name = "full";
	cdev->flags = DEVFS_IS_CHARACTER_DEV;
	cdev->ops = &fullops;

	devfs_create(cdev);

	return 0;
}

device_initcall(full_init);

static ssize_t null_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	return count;
}

static struct file_operations nullops = {
	.write = null_write,
	.lseek = dev_lseek_default,
};

static int null_init(void)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));

	cdev->name = "null";
	cdev->flags = DEVFS_IS_CHARACTER_DEV;
	cdev->ops = &nullops;

	devfs_create(cdev);

	return 0;
}

device_initcall(null_init);

static ssize_t prng_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	get_random_bytes(buf, count);
	return count;
}

static struct file_operations prngops = {
	.read  = prng_read,
	.lseek = dev_lseek_default,
};

static int prng_init(void)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));

	cdev->name = "prng";
	cdev->flags = DEVFS_IS_CHARACTER_DEV;
	cdev->ops = &prngops;

	devfs_create(cdev);

	return 0;
}

device_initcall(prng_init);

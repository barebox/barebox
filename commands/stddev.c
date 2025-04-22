// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <common.h>
#include <init.h>
#include <stdlib.h>

static ssize_t zero_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	memset(buf, 0, count);
	return count;
}

static struct cdev_operations zeroops = {
	.read  = zero_read,
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

static struct cdev_operations fullops = {
	.read  = full_read,
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

static ssize_t null_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	return 0;
}

static struct cdev_operations nullops = {
	.write = null_write,
	.read = null_read,
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

static struct cdev_operations prngops = {
	.read  = prng_read,
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

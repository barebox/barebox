/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *               2009 Marc Kleine-Budde <mkl@pengutronix.de>
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
 *
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>

#include <i2c/i2c.h>
#include <mfd/mc9sdz60.h>

#define DRIVERNAME		"mc9sdz60"

#define to_mc9sdz60(a)		container_of(a, struct mc9sdz60, cdev)

static struct mc9sdz60 *mc_dev;

struct mc9sdz60 *mc9sdz60_get(void)
{
	if (!mc_dev)
		return NULL;

	return mc_dev;
}
EXPORT_SYMBOL(mc9sdz60_get);

int mc9sdz60_reg_read(struct mc9sdz60 *mc9sdz60, enum mc9sdz60_reg reg, u8 *val)
{
	int ret;

	ret = i2c_read_reg(mc9sdz60->client, reg, val, 1);

	return ret == 1 ? 0 : ret;
}
EXPORT_SYMBOL(mc9sdz60_reg_read);

int mc9sdz60_reg_write(struct mc9sdz60 *mc9sdz60, enum mc9sdz60_reg reg, u8 val)
{
	int ret;

	ret = i2c_write_reg(mc9sdz60->client, reg, &val, 1);

	return ret == 1 ? 0 : ret;
}
EXPORT_SYMBOL(mc9sdz60_reg_write);

int mc9sdz60_set_bits(struct mc9sdz60 *mc9sdz60, enum mc9sdz60_reg reg, u8 mask, u8 val)
{
	u8 tmp;
	int err;

	err = mc9sdz60_reg_read(mc9sdz60, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = mc9sdz60_reg_write(mc9sdz60, reg, tmp);

	return err;
}
EXPORT_SYMBOL(mc9sdz60_set_bits);

static ssize_t mc_read(struct cdev *cdev, void *_buf, size_t count, loff_t offset, ulong flags)
{
	struct mc9sdz60 *mc9sdz60 = to_mc9sdz60(cdev);
	u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = mc9sdz60_reg_read(mc9sdz60, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t mc_write(struct cdev *cdev, const void *_buf, size_t count, loff_t offset, ulong flags)
{
	struct mc9sdz60 *mc9sdz60 = to_mc9sdz60(cdev);
	const u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = mc9sdz60_reg_write(mc9sdz60, offset, *buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct cdev_operations mc_fops = {
	.lseek	= dev_lseek_default,
	.read	= mc_read,
	.write	= mc_write,
};

static int mc_probe(struct device_d *dev)
{
	if (mc_dev)
		return -EBUSY;

	mc_dev = xzalloc(sizeof(struct mc9sdz60));
	mc_dev->cdev.name = DRIVERNAME;
	mc_dev->client = to_i2c_client(dev);
	mc_dev->cdev.size = 64;		/* 35 known registers */
	mc_dev->cdev.dev = dev;
	mc_dev->cdev.ops = &mc_fops;

	devfs_create(&mc_dev->cdev);

	return 0;
}

static struct driver_d mc_driver = {
	.name  = DRIVERNAME,
	.probe = mc_probe,
};
device_i2c_driver(mc_driver);

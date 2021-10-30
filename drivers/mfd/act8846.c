// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *               2009 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * Copied from drivers/mfd/mc9sdz60.c
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>

#include <i2c/i2c.h>
#include <mfd/act8846.h>

#define DRIVERNAME		"act8846"

#define to_act8846(a)		container_of(a, struct act8846, cdev)

static struct act8846 *act8846_dev;

struct act8846 *act8846_get(void)
{
	if (!act8846_dev)
		return NULL;

	return act8846_dev;
}
EXPORT_SYMBOL(act8846_get);

int act8846_reg_read(struct act8846 *act8846, enum act8846_reg reg, u8 *val)
{
	int ret;

	ret = i2c_read_reg(act8846->client, reg, val, 1);

	return ret == 1 ? 0 : ret;
}
EXPORT_SYMBOL(act8846_reg_read);

int act8846_reg_write(struct act8846 *act8846, enum act8846_reg reg, u8 val)
{
	int ret;

	ret = i2c_write_reg(act8846->client, reg, &val, 1);

	return ret == 1 ? 0 : ret;
}
EXPORT_SYMBOL(act8846_reg_write);

int act8846_set_bits(struct act8846 *act8846, enum act8846_reg reg,
		     u8 mask, u8 val)
{
	u8 tmp;
	int err;

	err = act8846_reg_read(act8846, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = act8846_reg_write(act8846, reg, tmp);

	return err;
}
EXPORT_SYMBOL(act8846_set_bits);

static ssize_t act8846_read(struct cdev *cdev, void *_buf, size_t count,
			    loff_t offset, ulong flags)
{
	struct act8846 *act8846 = to_act8846(cdev);
	u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = act8846_reg_read(act8846, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t act8846_write(struct cdev *cdev, const void *_buf, size_t count,
			     loff_t offset, ulong flags)
{
	struct act8846 *act8846 = to_act8846(cdev);
	const u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = act8846_reg_write(act8846, offset, *buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct cdev_operations act8846_fops = {
	.read	= act8846_read,
	.write	= act8846_write,
};

static int act8846_probe(struct device_d *dev)
{
	if (act8846_dev)
		return -EBUSY;

	act8846_dev = xzalloc(sizeof(struct act8846));
	act8846_dev->cdev.name = DRIVERNAME;
	act8846_dev->client = to_i2c_client(dev);
	act8846_dev->cdev.size = 64;
	act8846_dev->cdev.dev = dev;
	act8846_dev->cdev.ops = &act8846_fops;

	devfs_create(&act8846_dev->cdev);

	return 0;
}

static struct driver_d act8846_driver = {
	.name  = DRIVERNAME,
	.probe = act8846_probe,
};
device_i2c_driver(act8846_driver);

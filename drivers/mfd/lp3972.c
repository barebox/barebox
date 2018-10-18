/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *               2009 Marc Kleine-Budde <mkl@pengutronix.de>
 *               2009 Eric Benard <eric@eukrea.com>
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

#include <asm/byteorder.h>

#define DRIVERNAME		"lp3972"

struct lp_priv {
	struct cdev		cdev;
	struct i2c_client	*client;
};

#define to_lp_priv(a)		container_of(a, struct lp_priv, cdev)

static struct lp_priv *lp_dev;

struct i2c_client *lp3972_get_client(void)
{
	if (!lp_dev)
		return NULL;

	return lp_dev->client;
}

static u32 lp_read_reg(struct lp_priv *lp, int reg)
{
	u8 buf;

	i2c_read_reg(lp->client, reg, &buf, sizeof(buf));

	return buf;
}

static ssize_t lp_read(struct cdev *cdev, void *_buf, size_t count, loff_t offset, ulong flags)
{
	struct lp_priv *priv = to_lp_priv(cdev);
	int i = count;
	u8 *buf = _buf;

	while (i) {
		*buf = lp_read_reg(priv, offset);
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct cdev_operations lp_fops = {
	.lseek	= dev_lseek_default,
	.read	= lp_read,
};

static int lp_probe(struct device_d *dev)
{
	if (lp_dev)
		return -EBUSY;

	lp_dev = xzalloc(sizeof(struct lp_priv));
	lp_dev->cdev.name = DRIVERNAME;
	lp_dev->client = to_i2c_client(dev);
	lp_dev->cdev.size = 256;
	lp_dev->cdev.dev = dev;
	lp_dev->cdev.ops = &lp_fops;

	devfs_create(&lp_dev->cdev);

	return 0;
}

static struct driver_d lp_driver = {
	.name  = DRIVERNAME,
	.probe = lp_probe,
};
device_i2c_driver(lp_driver);

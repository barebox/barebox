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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>

#include <i2c/i2c.h>

#include <asm/byteorder.h>

#define DRIVERNAME		"mc13892"

struct mc_priv {
	struct cdev		cdev;
	struct i2c_client	*client;
};

#define to_mc_priv(a)		container_of(a, struct mc_priv, cdev)

static struct mc_priv *mc_dev;

struct i2c_client *mc13892_get_client(void)
{
	if (!mc_dev)
		return NULL;

	return mc_dev->client;
}

static u32 mc_read_reg(struct mc_priv *mc, int reg)
{
	u32 buf;

	i2c_read_reg(mc->client, reg, (u8 *)&buf, sizeof(buf));

	return buf;
}

static ssize_t mc_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	struct mc_priv *priv = to_mc_priv(cdev);
	int i = count >> 2;
	u32 *buf = _buf;

	offset >>= 2;

	while (i) {
		*buf = mc_read_reg(priv, offset);
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct file_operations mc_fops = {
	.lseek	= dev_lseek_default,
	.read	= mc_read,
};

static int mc_probe(struct device_d *dev)
{
	if (mc_dev)
		return -EBUSY;

	mc_dev = xzalloc(sizeof(struct mc_priv));
	mc_dev->cdev.name = DRIVERNAME;
	mc_dev->client = to_i2c_client(dev);
	mc_dev->cdev.size = 256;
	mc_dev->cdev.dev = dev;
	mc_dev->cdev.ops = &mc_fops;

	devfs_create(&mc_dev->cdev);

	return 0;
}

static struct driver_d mc_driver = {
	.name  = DRIVERNAME,
	.probe = mc_probe,
};

static int mc_init(void)
{
        register_driver(&mc_driver);
        return 0;
}

device_initcall(mc_init);

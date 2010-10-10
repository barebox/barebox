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
#include <mfd/mc13892.h>

#define DRIVERNAME		"mc13892"

#define to_mc13892(a)		container_of(a, struct mc13892, cdev)

static struct mc13892 *mc_dev;

struct mc13892 *mc13892_get(void)
{
	if (!mc_dev)
		return NULL;

	return mc_dev;
}
EXPORT_SYMBOL(mc13892_get);

int mc13892_reg_read(struct mc13892 *mc13892, enum mc13892_reg reg, u32 *val)
{
	u8 buf[3];
	int ret;

	ret = i2c_read_reg(mc13892->client, reg, buf, 3);
	*val = buf[0] << 16 | buf[1] << 8 | buf[2] << 0;

	return ret == 3 ? 0 : ret;
}
EXPORT_SYMBOL(mc13892_reg_read)

int mc13892_reg_write(struct mc13892 *mc13892, enum mc13892_reg reg, u32 val)
{
	u8 buf[] = {
		val >> 16,
		val >>  8,
		val >>  0,
	};
	int ret;

	ret = i2c_write_reg(mc13892->client, reg, buf, 3);

	return ret == 3 ? 0 : ret;
}
EXPORT_SYMBOL(mc13892_reg_write)

int mc13892_set_bits(struct mc13892 *mc13892, enum mc13892_reg reg, u32 mask, u32 val)
{
	u32 tmp;
	int err;

	err = mc13892_reg_read(mc13892, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = mc13892_reg_write(mc13892, reg, tmp);

	return err;
}
EXPORT_SYMBOL(mc13892_set_bits);

static ssize_t mc_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	struct mc13892 *priv = to_mc13892(cdev);
	u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc13892_reg_read(priv, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t mc_write(struct cdev *cdev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	struct mc13892 *mc13892 = to_mc13892(cdev);
	const u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc13892_reg_write(mc13892, offset, *buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct file_operations mc_fops = {
	.lseek	= dev_lseek_default,
	.read	= mc_read,
	.write	= mc_write,
};

static int mc_probe(struct device_d *dev)
{
	if (mc_dev)
		return -EBUSY;

	mc_dev = xzalloc(sizeof(struct mc13892));
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

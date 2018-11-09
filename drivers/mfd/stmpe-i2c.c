/*
 * Copyright (C) 2012 Pengutronix
 * Steffen Trumtrar <s.trumtrar@pengutronix.de>
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
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <of.h>

#include <i2c/i2c.h>
#include <mfd/stmpe-i2c.h>

#define DRIVERNAME		"stmpe-i2c"

#define to_stmpe(a)		container_of(a, struct stmpe, cdev)

int stmpe_reg_read(struct stmpe *stmpe, u32 reg, u8 *val)
{
	int ret;

	ret = i2c_read_reg(stmpe->client, reg, val, 1);

	return ret == 1 ? 0 : ret;
}
EXPORT_SYMBOL(stmpe_reg_read);

int stmpe_reg_write(struct stmpe *stmpe, u32 reg, u8 val)
{
	int ret;

	ret = i2c_write_reg(stmpe->client, reg, &val, 1);

	return ret == 1 ? 0 : ret;
}
EXPORT_SYMBOL(stmpe_reg_write);

int stmpe_set_bits(struct stmpe *stmpe, u32 reg, u8 mask, u8 val)
{
	u8 tmp;
	int err;

	err = stmpe_reg_read(stmpe, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = stmpe_reg_write(stmpe, reg, tmp);

	return err;
}
EXPORT_SYMBOL(stmpe_set_bits);

static ssize_t stmpe_read(struct cdev *cdev, void *_buf, size_t count, loff_t offset, ulong flags)
{
	struct stmpe *stmpe = to_stmpe(cdev);
	u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = stmpe_reg_read(stmpe, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t stmpe_write(struct cdev *cdev, const void *_buf, size_t count, loff_t offset, ulong flags)
{
	struct stmpe *stmpe = to_stmpe(cdev);
	const u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = stmpe_reg_write(stmpe, offset, *buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct cdev_operations stmpe_fops = {
	.lseek	= dev_lseek_default,
	.read	= stmpe_read,
	.write	= stmpe_write,
};

static struct stmpe_platform_data *stmpe_of_probe(struct device_d *dev)
{
	struct stmpe_platform_data *pdata;
	struct device_node *node;

	if (!IS_ENABLED(CONFIG_OFDEVICE) || !dev->device_node)
		return NULL;

	pdata = xzalloc(sizeof(*pdata));

	for_each_child_of_node(dev->device_node, node) {
		if (!strcmp(node->name, "stmpe_gpio")) {
			pdata->blocks |= STMPE_BLOCK_GPIO;
		}
	}

	return pdata;
}

static int stmpe_probe(struct device_d *dev)
{
	struct stmpe_platform_data *pdata = dev->platform_data;
	struct stmpe *stmpe_dev;
	struct stmpe_client_info *i2c_ci;

	if (!pdata) {
		pdata = stmpe_of_probe(dev);
		if (!pdata) {
			dev_dbg(dev, "no platform data\n");
			return -ENODEV;
		}
	}

	stmpe_dev = xzalloc(sizeof(struct stmpe));
	stmpe_dev->cdev.name = basprintf(DRIVERNAME"%d",
					   cdev_find_free_index(DRIVERNAME));
	stmpe_dev->client = to_i2c_client(dev);
	stmpe_dev->cdev.size = 191;		/* 191 known registers */
	stmpe_dev->cdev.dev = dev;
	stmpe_dev->cdev.ops = &stmpe_fops;
	stmpe_dev->pdata = pdata;
	dev->priv = stmpe_dev;

	i2c_ci = xzalloc(sizeof(struct stmpe_client_info));
	i2c_ci->stmpe = stmpe_dev;
	i2c_ci->read_reg = stmpe_reg_read;
	i2c_ci->write_reg = stmpe_reg_write;

	if (pdata->blocks & STMPE_BLOCK_GPIO)
		add_generic_device("stmpe-gpio", DEVICE_ID_DYNAMIC, NULL, 0, 0, IORESOURCE_MEM, i2c_ci);

	devfs_create(&stmpe_dev->cdev);

	return 0;
}

static struct platform_device_id stmpe_i2c_id[] = {
	{ "stmpe1601", 0 },
	{ }
};

static struct driver_d stmpe_driver = {
	.name  = DRIVERNAME,
	.probe = stmpe_probe,
	.id_table = stmpe_i2c_id,
};
device_i2c_driver(stmpe_driver);

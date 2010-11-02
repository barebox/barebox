/*
 * Copyright (C) 2010 Michael Grzeschik <mgr@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>

#include <i2c/i2c.h>
#include <mfd/twl4030.h>

#define DRIVERNAME		"twl4030"

#define to_twl4030(a)		container_of(a, struct twl4030, cdev)

static struct twl4030 *twl_dev;

struct twl4030 *twl4030_get(void)
{
	if (!twl_dev)
		return NULL;

	return twl_dev;
}
EXPORT_SYMBOL(twl4030_get);

int twl4030_reg_read(struct twl4030 *twl4030, u16 reg, u8 *val)
{
	int ret;
	struct i2c_msg xfer_msg[2];
	struct i2c_msg *msg;
	int i2c_addr;
	unsigned char buf = reg & 0xff;

	i2c_addr = twl4030->client->addr + (reg / 0x100);

	/* [MSG1] fill the register address data */
	msg = &xfer_msg[0];
	msg->addr = i2c_addr;
	msg->len = 1;
	msg->flags = 0;	/* Read the register value */
	msg->buf = &buf;
	/* [MSG2] fill the data rx buffer */
	msg = &xfer_msg[1];
	msg->addr = i2c_addr;
	msg->flags = I2C_M_RD;	/* Read the register value */
	msg->len = 1;	/* only n bytes */
	msg->buf = val;
	ret = i2c_transfer(twl4030->client->adapter, xfer_msg, 2);

	/* i2c_transfer returns number of messages transferred */
	if (ret < 0) {
		pr_err("%s: failed to transfer all messages: %s\n", __func__, strerror(-ret));
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL(twl4030_reg_read)

int twl4030_reg_write(struct twl4030 *twl4030, u16 reg, u8 val)
{
	int ret;
	struct i2c_msg xfer_msg[1];
	struct i2c_msg *msg;
	int i2c_addr;
	u8 buf[2];

	buf[0] = reg & 0xff;
	buf[1] = val;

	i2c_addr = twl4030->client->addr + (reg / 0x100);

	/*
	 * [MSG1]: fill the register address data
	 * fill the data Tx buffer
	 */
	msg = xfer_msg;
	msg->addr = i2c_addr;
	msg->len = 2;
	msg->flags = 0;
	msg->buf = buf;
	/* over write the first byte of buffer with the register address */
	ret = i2c_transfer(twl4030->client->adapter, xfer_msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret < 0) {
		pr_err("%s: failed to transfer all messages: %s\n", __func__, strerror(-ret));
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL(twl4030_reg_write)

int twl4030_set_bits(struct twl4030 *twl4030, enum twl4030_reg reg, u8 mask, u8 val)
{
	u8 tmp;
	int err;

	err = twl4030_reg_read(twl4030, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = twl4030_reg_write(twl4030, reg, tmp);

	return err;
}
EXPORT_SYMBOL(twl4030_set_bits);

static ssize_t twl_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	struct twl4030 *priv = to_twl4030(cdev);
	u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = twl4030_reg_read(priv, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t twl_write(struct cdev *cdev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	struct twl4030 *twl4030 = to_twl4030(cdev);
	const u8 *buf = _buf;
	size_t i = count;
	int err;

	while (i) {
		err = twl4030_reg_write(twl4030, offset, *buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct file_operations twl_fops = {
	.lseek	= dev_lseek_default,
	.read	= twl_read,
	.write	= twl_write,
};

static int twl_probe(struct device_d *dev)
{
	if (twl_dev)
		return -EBUSY;

	twl_dev = xzalloc(sizeof(struct twl4030));
	twl_dev->cdev.name = DRIVERNAME;
	twl_dev->client = to_i2c_client(dev);
	twl_dev->cdev.size = 1024;
	twl_dev->cdev.dev = dev;
	twl_dev->cdev.ops = &twl_fops;

	devfs_create(&twl_dev->cdev);

	return 0;
}

static struct driver_d twl_driver = {
	.name  = DRIVERNAME,
	.probe = twl_probe,
};

static int twl_init(void)
{
        register_driver(&twl_driver);
        return 0;
}

device_initcall(twl_init);

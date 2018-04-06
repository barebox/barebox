/*
 * Copyright (C) 2011 Alexander Aring <a.aring@phytec.de>
 *
 * Based on:
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
#include <mfd/twl-core.h>

#define to_twlcore(a)       container_of(a, struct twlcore, cdev)

static struct twlcore *twl_dev;

struct twlcore *twlcore_get(void)
{
	return twl_dev;
}
EXPORT_SYMBOL(twlcore_get);

int twlcore_reg_read(struct twlcore *twlcore, u16 reg, u8 *val)
{
	int ret;
	struct i2c_msg xfer_msg[2];
	struct i2c_msg *msg;
	int i2c_addr;
	unsigned char buf = reg & 0xff;

	i2c_addr = twlcore->client->addr + (reg / 0x100);

	/* [MSG1] fill the register address data */
	msg = &xfer_msg[0];
	msg->addr = i2c_addr;
	msg->len = 1;
	msg->flags = 0;
	msg->buf = &buf;
	/* [MSG2] fill the data rx buffer */
	msg = &xfer_msg[1];
	msg->addr = i2c_addr;
	msg->flags = I2C_M_RD;
	msg->len = 1;	/* only n bytes */
	msg->buf = val;
	ret = i2c_transfer(twlcore->client->adapter, xfer_msg, 2);

	/* i2c_transfer returns number of messages transferred */
	if (ret < 0) {
		pr_err("%s: failed to transfer all messages: %s\n",
				__func__, strerror(-ret));
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL(twlcore_reg_read);

int twlcore_reg_write(struct twlcore *twlcore, u16 reg, u8 val)
{
	int ret;
	struct i2c_msg xfer_msg[1];
	struct i2c_msg *msg;
	int i2c_addr;
	u8 buf[2];

	buf[0] = reg & 0xff;
	buf[1] = val;

	i2c_addr = twlcore->client->addr + (reg / 0x100);

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
	ret = i2c_transfer(twlcore->client->adapter, xfer_msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret < 0) {
		pr_err("%s: failed to transfer all messages: %s\n",
				__func__, strerror(-ret));
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL(twlcore_reg_write);

int twlcore_set_bits(struct twlcore *twlcore, u16 reg, u8 mask, u8 val)
{
	u8 tmp;
	int ret;

	ret = twlcore_reg_read(twlcore, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (ret)
		ret = twlcore_reg_write(twlcore, reg, tmp);

	return ret;
}
EXPORT_SYMBOL(twlcore_set_bits);

static ssize_t twl_read(struct cdev *cdev, void *_buf, size_t count,
		loff_t offset, ulong flags)
{
	struct twlcore *priv = to_twlcore(cdev);
	u8 *buf = _buf;
	size_t i;
	int ret;

	for (i = 0; i < count; i++) {
		ret = twlcore_reg_read(priv, offset, buf);
		if (ret)
			return (ssize_t)ret;
		buf++;
		offset++;
	}

	return count;
}

static ssize_t twl_write(struct cdev *cdev, const void *_buf, size_t count,
		loff_t offset, ulong flags)
{
	struct twlcore *twlcore = to_twlcore(cdev);
	const u8 *buf = _buf;
	size_t i;
	int ret;

	for (i = 0; i < count; i++) {
		ret = twlcore_reg_write(twlcore, offset, *buf);
		if (ret)
			return (ssize_t)ret;
		buf++;
		offset++;
	}

	return count;
}

struct cdev_operations twl_fops = {
	.lseek	= dev_lseek_default,
	.read	= twl_read,
	.write	= twl_write,
};
EXPORT_SYMBOL(twl_fops);

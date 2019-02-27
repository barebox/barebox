/*
 * Copyright 2013 GE Intelligent Platforms, Inc
 * Copyright 2006,2009 Freescale Semiconductor, Inc.
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
 * Early I2C support functions to read SPD data or board
 * information.
 * Based on U-Boot drivers/i2c/fsl_i2c.c
 */
#include <common.h>
#include <i2c/i2c.h>
#include <i2c/i2c-early.h>

#include "i2c-imx.h"

struct fsl_i2c {
	void __iomem *regs;
	unsigned int i2cr_ien_opcode;
	unsigned int i2sr_clr_opcode;
	unsigned int ifdr;
	unsigned int regshift;
};

static inline void fsl_i2c_write_reg(unsigned int val,
				     struct fsl_i2c *fsl_i2c,
				     unsigned int reg)
{
	reg <<= fsl_i2c->regshift;

	writeb(val, fsl_i2c->regs + reg);
}

static inline unsigned char fsl_i2c_read_reg(struct fsl_i2c *fsl_i2c,
					     unsigned int reg)
{
	reg <<= fsl_i2c->regshift;

	return readb(fsl_i2c->regs + reg);
}

static int i2c_fsl_poll_status(struct fsl_i2c *fsl_i2c, uint8_t set, uint8_t clear)
{
	int timeout = 1000000;
	uint8_t temp;

	while (1) {
		temp = fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2SR);
		if (temp & set)
			return 0;
		if (~temp & clear)
			return 0;

		if (!--timeout) {
			pr_debug("timeout waiting for status %s 0x%02x, cur status: 0x%02x\n",
					set ? "set" : "clear",
					set ? set : clear,
					temp);
			return -EIO;
		}
	}
}

static int i2c_fsl_bus_busy(struct fsl_i2c *fsl_i2c)
{
	return i2c_fsl_poll_status(fsl_i2c, I2SR_IBB, 0);
}

static int i2c_fsl_bus_idle(struct fsl_i2c *fsl_i2c)
{
	return i2c_fsl_poll_status(fsl_i2c, 0, I2SR_IBB);
}

static int i2c_fsl_trx_complete(struct fsl_i2c *fsl_i2c)
{
	int ret;

	ret = i2c_fsl_poll_status(fsl_i2c, I2SR_IIF, 0);
	if (ret)
		return ret;

	fsl_i2c_write_reg(fsl_i2c->i2sr_clr_opcode,
			  fsl_i2c, FSL_I2C_I2SR);

	return 0;
}

static int i2c_fsl_acked(struct fsl_i2c *fsl_i2c)
{
	return i2c_fsl_poll_status(fsl_i2c, 0, I2SR_RXAK);
}

static int i2c_fsl_start(struct fsl_i2c *fsl_i2c)
{
	unsigned int temp = 0;
	int ret;

	fsl_i2c_write_reg(fsl_i2c->ifdr, fsl_i2c, FSL_I2C_IFDR);

	/* Enable I2C controller */
	fsl_i2c_write_reg(fsl_i2c->i2sr_clr_opcode,
			  fsl_i2c, FSL_I2C_I2SR);
	fsl_i2c_write_reg(fsl_i2c->i2cr_ien_opcode,
			  fsl_i2c, FSL_I2C_I2CR);

	/* Wait controller to be stable */
	udelay(100);

	/* Start I2C transaction */
	temp = fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2CR);
	temp |= I2CR_MSTA;
	fsl_i2c_write_reg(temp, fsl_i2c, FSL_I2C_I2CR);

	ret = i2c_fsl_bus_busy(fsl_i2c);
	if (ret)
		return -EAGAIN;

	temp |= I2CR_MTX | I2CR_TXAK;
	fsl_i2c_write_reg(temp, fsl_i2c, FSL_I2C_I2CR);

	return ret;
}

static void i2c_fsl_stop(struct fsl_i2c *fsl_i2c)
{
	unsigned int temp = 0;

	/* Stop I2C transaction */
	temp = fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2CR);
	temp &= ~(I2CR_MSTA | I2CR_MTX);
	fsl_i2c_write_reg(temp, fsl_i2c, FSL_I2C_I2CR);
	/* wait for the stop condition to be send, otherwise the i2c
	 * controller is disabled before the STOP is sent completely */

	i2c_fsl_bus_idle(fsl_i2c);
}

static int i2c_fsl_send(struct fsl_i2c *fsl_i2c, uint8_t data)
{
	int ret;

	pr_debug("%s send 0x%02x\n", __func__, data);

	fsl_i2c_write_reg(data, fsl_i2c, FSL_I2C_I2DR);

	ret = i2c_fsl_trx_complete(fsl_i2c);
	if (ret) {
		pr_debug("%s timeout 1\n", __func__);
		return ret;
	}

	ret = i2c_fsl_acked(fsl_i2c);
	if (ret) {
		pr_debug("%s timeout 2\n", __func__);
		return ret;
	}

	return 0;
}

static int i2c_fsl_write(struct fsl_i2c *fsl_i2c, struct i2c_msg *msg)
{
	int i, ret;

	if (!(msg->flags & I2C_M_DATA_ONLY)) {
		ret = i2c_fsl_send(fsl_i2c, msg->addr << 1);
		if (ret)
			return ret;
	}

	/* write data */
	for (i = 0; i < msg->len; i++) {
		ret = i2c_fsl_send(fsl_i2c, msg->buf[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int i2c_fsl_read(struct fsl_i2c *fsl_i2c, struct i2c_msg *msg)
{
	int i, ret;
	unsigned int temp;

	/* clear IIF */
	fsl_i2c_write_reg(fsl_i2c->i2sr_clr_opcode,
			  fsl_i2c, FSL_I2C_I2SR);

	if (!(msg->flags & I2C_M_DATA_ONLY)) {
		ret = i2c_fsl_send(fsl_i2c, (msg->addr << 1) | 1);
		if (ret)
			return ret;
	}

	/* setup bus to read data */
	temp = fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2CR);
	temp &= ~I2CR_MTX;
	if (msg->len - 1)
		temp &= ~I2CR_TXAK;
	fsl_i2c_write_reg(temp, fsl_i2c, FSL_I2C_I2CR);

	fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2DR); /* dummy read */

	/* read data */
	for (i = 0; i < msg->len; i++) {
		ret = i2c_fsl_trx_complete(fsl_i2c);
		if (ret)
			return ret;

		if (i == (msg->len - 1)) {
			i2c_fsl_stop(fsl_i2c);
		} else if (i == (msg->len - 2)) {
			temp = fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2CR);
			temp |= I2CR_TXAK;
			fsl_i2c_write_reg(temp, fsl_i2c, FSL_I2C_I2CR);
		}
		msg->buf[i] = fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2DR);
	}
	return 0;
}

/**
 * i2c_fsl_xfer - transfer I2C messages on i.MX compatible I2C controllers
 * @ctx: driver context pointer
 * @msgs: pointer to I2C messages
 * @num: number of messages to transfer
 *
 * This function transfers I2C messages on i.MX and compatible I2C controllers.
 * If successful returns the number of messages transferred, otherwise a negative
 * error code is returned.
 */
int i2c_fsl_xfer(void *ctx, struct i2c_msg *msgs, int num)
{
	struct fsl_i2c *fsl_i2c = ctx;
	unsigned int i, temp;
	int ret;

	pr_debug("%s enter\n", __func__);

	/* Start I2C transfer */
	for (i = 0; i < 3; i++) {
		ret = i2c_fsl_start(fsl_i2c);
		if (!ret)
			break;
		if (ret == -EAGAIN)
			continue;
		return ret;
	}

	/* read/write data */
	for (i = 0; i < num; i++) {
		if (i && !(msgs[i].flags & I2C_M_DATA_ONLY)) {
			temp = fsl_i2c_read_reg(fsl_i2c, FSL_I2C_I2CR);
			temp |= I2CR_RSTA;
			fsl_i2c_write_reg(temp, fsl_i2c, FSL_I2C_I2CR);

			ret = i2c_fsl_bus_busy(fsl_i2c);
			if (ret)
				goto fail0;
		}

		/* write/read data */
		if (msgs[i].flags & I2C_M_RD)
			ret = i2c_fsl_read(fsl_i2c, &msgs[i]);
		else
			ret = i2c_fsl_write(fsl_i2c, &msgs[i]);
		if (ret)
			goto fail0;
	}

fail0:
	/* Stop I2C transfer */
	i2c_fsl_stop(fsl_i2c);

	/* Disable I2C controller, and force our state to stopped */
	temp = fsl_i2c->i2cr_ien_opcode ^ I2CR_IEN,
	fsl_i2c_write_reg(temp, fsl_i2c, FSL_I2C_I2CR);

	return (ret < 0) ? ret : num;
}

static struct fsl_i2c fsl_i2c;

/**
 * ls1046_i2c_init - Return a context pointer for accessing I2C on LS1046a
 * @regs: The base address of the I2C controller to access
 *
 * This function returns a context pointer suitable to transfer I2C messages
 * using i2c_fsl_xfer.
 */
void *ls1046_i2c_init(void __iomem *regs)
{
	fsl_i2c.regs = regs;
	fsl_i2c.regshift = 0;
	fsl_i2c.i2cr_ien_opcode = I2CR_IEN_OPCODE_0;
	fsl_i2c.i2sr_clr_opcode = I2SR_CLR_OPCODE_W1C;
	/* Divider for ~100kHz when coming from the ROM */
	fsl_i2c.ifdr = 0x3e;

	return &fsl_i2c;
}

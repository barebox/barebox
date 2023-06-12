// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2012 GE Intelligent Platforms, Inc
 * Copyright (C) 2002 Motorola GSG-China
 *               2009 Marc Kleine-Budde, Pengutronix
 *
 * Author:
 *	Darius Augulis, Teltonika Inc.
 *
 * Desc.:
 *  Implementation of I2C Adapter/Algorithm Driver
 *  for I2C Bus integrated in Freescale i.MX/MXC processors and
 *  85xx processors.
 *
 * Derived from Motorola GSG China I2C example driver
 *
 * Copyright (C) 2005 Torsten Koschorrek <koschorrek at synertronixx.de
 * Copyright (C) 2005 Matthias Blaschke <blaschke at synertronixx.de
 * Copyright (C) 2007 RightHand Technologies, Inc.
 * Copyright (C) 2008 Darius Augulis <darius.augulis at teltonika.lt>
 *
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <gpio.h>
#include <malloc.h>
#include <types.h>
#include <xfuncs.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <pinctrl.h>
#include <of_gpio.h>
#include <of_device.h>

#include <io.h>
#include <i2c/i2c.h>

#include "i2c-imx.h"

/*
 * sorted list of clock divider, register value pairs
 * taken from table 26-5, p.26-9, Freescale i.MX
 * Integrated Portable System Processor Reference Manual
 * Document Number: MC9328MXLRM, Rev. 5.1, 06/2007
 *
 * Duplicated divider values removed from list
 */
struct fsl_i2c_clk_pair {
	u16	div;
	u16	val;
};

static struct fsl_i2c_clk_pair imx_i2c_clk_div[] = {
	{ 22,	0x20 }, { 24,	0x21 }, { 26,	0x22 }, { 28,	0x23 },
	{ 30,	0x00 },	{ 32,	0x24 }, { 36,	0x25 }, { 40,	0x26 },
	{ 42,	0x03 }, { 44,	0x27 },	{ 48,	0x28 }, { 52,	0x05 },
	{ 56,	0x29 }, { 60,	0x06 }, { 64,	0x2A },	{ 72,	0x2B },
	{ 80,	0x2C }, { 88,	0x09 }, { 96,	0x2D }, { 104,	0x0A },
	{ 112,	0x2E }, { 128,	0x2F }, { 144,	0x0C }, { 160,	0x30 },
	{ 192,	0x31 },	{ 224,	0x32 }, { 240,	0x0F }, { 256,	0x33 },
	{ 288,	0x10 }, { 320,	0x34 },	{ 384,	0x35 }, { 448,	0x36 },
	{ 480,	0x13 }, { 512,	0x37 }, { 576,	0x14 },	{ 640,	0x38 },
	{ 768,	0x39 }, { 896,	0x3A }, { 960,	0x17 }, { 1024,	0x3B },
	{ 1152,	0x18 }, { 1280,	0x3C }, { 1536,	0x3D }, { 1792,	0x3E },
	{ 1920,	0x1B },	{ 2048,	0x3F }, { 2304,	0x1C }, { 2560,	0x1D },
	{ 3072,	0x1E }, { 3840,	0x1F }
};

/* Vybrid VF610 clock divider, register value pairs */
static struct fsl_i2c_clk_pair vf610_i2c_clk_div[] = {
	{ 20,   0x00 }, { 22,   0x01 }, { 24,   0x02 }, { 26,   0x03 },
	{ 28,   0x04 }, { 30,   0x05 }, { 32,   0x09 }, { 34,   0x06 },
	{ 36,   0x0A }, { 40,   0x07 }, { 44,   0x0C }, { 48,   0x0D },
	{ 52,   0x43 }, { 56,   0x0E }, { 60,   0x45 }, { 64,   0x12 },
	{ 68,   0x0F }, { 72,   0x13 }, { 80,   0x14 }, { 88,   0x15 },
	{ 96,   0x19 }, { 104,  0x16 }, { 112,  0x1A }, { 128,  0x17 },
	{ 136,  0x4F }, { 144,  0x1C }, { 160,  0x1D }, { 176,  0x55 },
	{ 192,  0x1E }, { 208,  0x56 }, { 224,  0x22 }, { 228,  0x24 },
	{ 240,  0x1F }, { 256,  0x23 }, { 288,  0x5C }, { 320,  0x25 },
	{ 384,  0x26 }, { 448,  0x2A }, { 480,  0x27 }, { 512,  0x2B },
	{ 576,  0x2C }, { 640,  0x2D }, { 768,  0x31 }, { 896,  0x32 },
	{ 960,  0x2F }, { 1024, 0x33 }, { 1152, 0x34 }, { 1280, 0x35 },
	{ 1536, 0x36 }, { 1792, 0x3A }, { 1920, 0x37 }, { 2048, 0x3B },
	{ 2304, 0x3C }, { 2560, 0x3D }, { 3072, 0x3E }, { 3584, 0x7A },
	{ 3840, 0x3F }, { 4096, 0x7B }, { 5120, 0x7D }, { 6144, 0x7E },
};

static const struct fsl_i2c_hwdata imx21_i2c_hwdata;

struct fsl_i2c_hwdata {
	unsigned		regshift;
	struct fsl_i2c_clk_pair	*clk_div;
	unsigned		ndivs;
	unsigned		i2sr_clr_opcode;
	unsigned		i2cr_ien_opcode;
};

struct fsl_i2c_struct {
	void __iomem		*base;
	struct clk		*clk;
	struct device		*dev;
	struct i2c_adapter	adapter;
	unsigned int 		disable_delay;
	unsigned int		ifdr;	/* FSL_I2C_IFDR */
	unsigned int		dfsrr;  /* FSL_I2C_DFSRR */
	struct i2c_bus_recovery_info rinfo;
	const struct fsl_i2c_hwdata *hwdata;
};
#define to_fsl_i2c_struct(a)	container_of(a, struct fsl_i2c_struct, adapter)

static inline void fsl_i2c_write_reg(unsigned int val,
				     struct fsl_i2c_struct *i2c_fsl,
				     unsigned int reg)
{
	reg <<= i2c_fsl->hwdata->regshift;
	writeb(val, i2c_fsl->base + reg);
}

static inline unsigned char fsl_i2c_read_reg(struct fsl_i2c_struct *i2c_fsl,
					     unsigned int reg)
{
	reg <<= i2c_fsl->hwdata->regshift;
	return readb(i2c_fsl->base + reg);
}

static void i2c_fsl_dump_reg(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	u32 reg_cr, reg_sr;

	reg_cr = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2CR);
	reg_sr = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2SR);

	dev_dbg(&adapter->dev, "CONTROL:\t"
		"IEN =%d, IIEN=%d, MSTA=%d, MTX =%d, TXAK=%d, RSTA=%d\n",
		(reg_cr & I2CR_IEN  ? 1 : 0), (reg_cr & I2CR_IIEN ? 1 : 0),
		(reg_cr & I2CR_MSTA ? 1 : 0), (reg_cr & I2CR_MTX  ? 1 : 0),
		(reg_cr & I2CR_TXAK ? 1 : 0), (reg_cr & I2CR_RSTA ? 1 : 0));

	dev_dbg(&adapter->dev, "STATUS:\t"
		"ICF =%d, IAAS=%d, IB  =%d, IAL =%d, SRW =%d, IIF =%d, RXAK=%d\n",
		(reg_sr & I2SR_ICF  ? 1 : 0), (reg_sr & I2SR_IAAS ? 1 : 0),
		(reg_sr & I2SR_IBB  ? 1 : 0), (reg_sr & I2SR_IAL  ? 1 : 0),
		(reg_sr & I2SR_SRW  ? 1 : 0), (reg_sr & I2SR_IIF  ? 1 : 0),
		(reg_sr & I2SR_RXAK ? 1 : 0));
}

static int i2c_fsl_poll_status(struct i2c_adapter *adapter, int timeout_ms,
			       uint8_t set, uint8_t clear)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	uint64_t start;
	uint8_t temp;

	start = get_time_ns();
	while (1) {
		temp = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2SR);
		if (temp & set)
			return 0;
		if (~temp & clear)
			return 0;

		if (is_timeout(start, timeout_ms * MSECOND)) {
			dev_dbg(&adapter->dev,
				 "timeout waiting for status %s 0x%02x, cur status: 0x%02x\n",
					set ? "set" : "clear",
					set ? set : clear,
					temp);
			return -EIO;
		}
	}
}

static int i2c_fsl_bus_busy(struct i2c_adapter *adapter)
{
	return i2c_fsl_poll_status(adapter, 500, I2SR_IBB, 0);
}

static int i2c_fsl_bus_idle(struct i2c_adapter *adapter)
{
	return i2c_fsl_poll_status(adapter, 500, 0, I2SR_IBB);
}

static int i2c_fsl_trx_complete(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	int ret;

	ret = i2c_fsl_poll_status(adapter, 100, I2SR_IIF, 0);
	if (ret)
		return ret;

	fsl_i2c_write_reg(i2c_fsl->hwdata->i2sr_clr_opcode,
			  i2c_fsl, FSL_I2C_I2SR);

	return 0;
}

static int i2c_fsl_acked(struct i2c_adapter *adapter)
{
	return i2c_fsl_poll_status(adapter, 5, 0, I2SR_RXAK);
}

static int i2c_fsl_start(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	unsigned int temp = 0;
	int result;

	fsl_i2c_write_reg(i2c_fsl->ifdr, i2c_fsl, FSL_I2C_IFDR);
#ifdef CONFIG_PPC
	if (i2c_fsl->dfsrr != -1)
		fsl_i2c_write_reg(i2c_fsl->dfsrr, i2c_fsl, FSL_I2C_DFSRR);
#endif

	/* Enable I2C controller */
	fsl_i2c_write_reg(i2c_fsl->hwdata->i2sr_clr_opcode,
			  i2c_fsl, FSL_I2C_I2SR);
	fsl_i2c_write_reg(i2c_fsl->hwdata->i2cr_ien_opcode,
			  i2c_fsl, FSL_I2C_I2CR);

	/* Wait controller to be stable */
	udelay(100);

	/* Start I2C transaction */
	temp = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2CR);
	temp |= I2CR_MSTA;
	fsl_i2c_write_reg(temp, i2c_fsl, FSL_I2C_I2CR);

	result = i2c_fsl_bus_busy(adapter);
	if (result) {
		result = i2c_recover_bus(&i2c_fsl->adapter);
		if (result)
			return result;
		return -EAGAIN;
	}

	temp |= I2CR_MTX | I2CR_TXAK;
	fsl_i2c_write_reg(temp, i2c_fsl, FSL_I2C_I2CR);

	return result;
}

static void i2c_fsl_stop(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	unsigned int temp = 0;

	/* Stop I2C transaction */
	temp = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2CR);
	if (!(temp & I2CR_MSTA))
		return;

	temp &= ~(I2CR_MSTA | I2CR_MTX);
	fsl_i2c_write_reg(temp, i2c_fsl, FSL_I2C_I2CR);
	/* wait for the stop condition to be send, otherwise the i2c
	 * controller is disabled before the STOP is sent completely */

	/* adding this delay helps on low bitrates */
	udelay(i2c_fsl->disable_delay);

	i2c_fsl_bus_idle(adapter);
}

#ifdef CONFIG_PPC

#include <mach/clock.h>

static void i2c_fsl_set_clk(struct fsl_i2c_struct *i2c_fsl,
				    unsigned int rate)
{
	unsigned int i2c_clk;
	unsigned short divider;
	/*
	 * We want to choose an FDR/DFSR that generates an I2C bus speed that
	 * is equal to or lower than the requested speed.  That means that we
	 * want the first divider that is equal to or greater than the
	 * calculated divider.
	 */
	u8 dfsr, fdr;
	/* a, b and dfsr matches identifiers A,B and C respectively in AN2919 */
	unsigned short a, b, ga, gb;
	unsigned long c_div, est_div;

	fdr = 0x31; /* Default if no FDR found */
	i2c_clk = fsl_get_i2c_freq();
	divider = min((unsigned short)(i2c_clk / rate), (unsigned short) -1);

	/*
	 * Condition 1: dfsr <= 50ns/T (T=period of I2C source clock in ns).
	 * or (dfsr * T) <= 50ns.
	 * Translate to dfsr = 5 * Frequency / 100,000,000
	 */
	dfsr = (5 * (i2c_clk / 1000)) / 100000;
	dev_dbg(i2c_fsl->dev,
		"<%s> requested speed:%d, i2c_clk:%d\n", __func__,
		rate, i2c_clk);
	if (!dfsr)
		dfsr = 1;

	est_div = ~0;
	for (ga = 0x4, a = 10; a <= 30; ga++, a += 2) {
		for (gb = 0; gb < 8; gb++) {
			b = 16 << gb;
			c_div = b * (a + ((3*dfsr)/b)*2);
			if ((c_div > divider) && (c_div < est_div)) {
				unsigned short bin_gb, bin_ga;

				est_div = c_div;
				bin_gb = gb << 2;
				bin_ga = (ga & 0x3) | ((ga & 0x4) << 3);
				fdr = bin_gb | bin_ga;
				rate = i2c_clk / est_div;
				dev_dbg(i2c_fsl->dev,
					"FDR:0x%.2x, div:%ld, ga:0x%x, gb:0x%x,"
					" a:%d, b:%d, speed:%d\n", fdr, est_div,
					ga, gb, a, b, rate);
				/* Condition 2 not accounted for */
				dev_dbg(i2c_fsl->dev,
					"Tr <= %d ns\n", (b - 3 * dfsr) *
					1000000 / (i2c_clk / 1000));
			}
		}
		if (a == 20)
			a += 2;
		if (a == 24)
			a += 4;
	}
	dev_dbg(i2c_fsl->dev,
		"divider:%d, est_div:%ld, DFSR:%d\n", divider, est_div, dfsr);
	dev_dbg(i2c_fsl->dev, "FDR:0x%.2x, speed:%d\n", fdr, rate);
	i2c_fsl->ifdr = fdr;
	i2c_fsl->dfsrr = dfsr;
}
#else
static void i2c_fsl_set_clk(struct fsl_i2c_struct *i2c_fsl,
			    unsigned int rate)
{
	struct fsl_i2c_clk_pair *i2c_clk_div = i2c_fsl->hwdata->clk_div;
	unsigned int i2c_clk_rate;
	unsigned int div;
	int i;

	/* Divider value calculation */
	i2c_clk_rate = clk_get_rate(i2c_fsl->clk);
	div = (i2c_clk_rate + rate - 1) / rate;
	if (div < i2c_clk_div[0].div)
		i = 0;
	else if (div > i2c_clk_div[i2c_fsl->hwdata->ndivs - 1].div)
		i = i2c_clk_div[i2c_fsl->hwdata->ndivs - 1].div - 1;
	else
		for (i = 0; i2c_clk_div[i].div < div; i++)
			;

	/* Store divider value */
	i2c_fsl->ifdr = i2c_clk_div[i].val;

	/*
	 * There dummy delay is calculated.
	 * It should be about one I2C clock period long.
	 * This delay is used in I2C bus disable function
	 * to fix chip hardware bug.
	 */
	i2c_fsl->disable_delay =
		(500000U * i2c_clk_div[i].div + (i2c_clk_rate / 2) - 1) /
		(i2c_clk_rate / 2);

	dev_dbg(i2c_fsl->dev, "<%s> I2C_CLK=%d, REQ DIV=%d\n",
		__func__, i2c_clk_rate, div);
	dev_dbg(i2c_fsl->dev, "<%s> IFDR[IC]=0x%x, REAL DIV=%d\n",
		__func__, i2c_clk_div[i].val, i2c_clk_div[i].div);
}
#endif

static int i2c_fsl_send(struct i2c_adapter *adapter, uint8_t data)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	int result;

	dev_dbg(&adapter->dev, "<%s> send 0x%02x\n", __func__, data);

	fsl_i2c_write_reg(data, i2c_fsl, FSL_I2C_I2DR);

	result = i2c_fsl_trx_complete(adapter);
	if (result)
		return result;

	return i2c_fsl_acked(adapter);
}

static int i2c_fsl_write(struct i2c_adapter *adapter, struct i2c_msg *msg)
{
	int i, result;

	if (!(msg->flags & I2C_M_DATA_ONLY)) {
		result = i2c_fsl_send(adapter, msg->addr << 1);
		if (result)
			return result;
	}

	/* write data */
	for (i = 0; i < msg->len; i++) {
		result = i2c_fsl_send(adapter, msg->buf[i]);
		if (result)
			return result;
	}

	return 0;
}

static int i2c_fsl_read(struct i2c_adapter *adapter, struct i2c_msg *msg,
			bool is_last)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	int i, result;
	unsigned int temp;

	/* clear IIF */
	fsl_i2c_write_reg(i2c_fsl->hwdata->i2sr_clr_opcode,
			  i2c_fsl, FSL_I2C_I2SR);

	if (!(msg->flags & I2C_M_DATA_ONLY)) {
		result = i2c_fsl_send(adapter, (msg->addr << 1) | 1);
		if (result)
			return result;
	}

	/* setup bus to read data */
	temp = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2CR);
	temp &= ~I2CR_MTX;
	if (msg->len - 1)
		temp &= ~I2CR_TXAK;
	fsl_i2c_write_reg(temp, i2c_fsl, FSL_I2C_I2CR);

	fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2DR); /* dummy read */

	/* read data */
	for (i = 0; i < msg->len; i++) {
		result = i2c_fsl_trx_complete(adapter);
		if (result)
			return result;

		if (is_last && i == msg->len - 1) {
			i2c_fsl_stop(adapter);
		} else if (i == (msg->len - 2)) {
			temp = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2CR);
			temp |= I2CR_TXAK;
			fsl_i2c_write_reg(temp, i2c_fsl, FSL_I2C_I2CR);
		}
		msg->buf[i] = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2DR);

		dev_dbg(&adapter->dev, "<%s> read byte: B%d=0x%02X\n",
			__func__, i, msg->buf[i]);
	}
	return 0;
}

static int i2c_fsl_xfer(struct i2c_adapter *adapter,
			struct i2c_msg *msgs, int num)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	unsigned int i, temp;
	int result;

	/* Start I2C transfer */
	for (i = 0; i < 3; i++) {
		result = i2c_fsl_start(adapter);
		if (!result)
			break;
		if (result == -EAGAIN)
			continue;
		return result;
	}

	/* read/write data */
	for (i = 0; i < num; i++) {
		if (i && !(msgs[i].flags & I2C_M_DATA_ONLY)) {
			temp = fsl_i2c_read_reg(i2c_fsl, FSL_I2C_I2CR);
			temp |= I2CR_RSTA;
			fsl_i2c_write_reg(temp, i2c_fsl, FSL_I2C_I2CR);

			result = i2c_fsl_bus_busy(adapter);
			if (result)
				goto fail0;
		}
		i2c_fsl_dump_reg(adapter);

		/* write/read data */
		if (msgs[i].flags & I2C_M_RD)
			result = i2c_fsl_read(adapter, &msgs[i], i == num - 1);
		else
			result = i2c_fsl_write(adapter, &msgs[i]);
		if (result)
			goto fail0;
	}

fail0:
	/* Stop I2C transfer */
	i2c_fsl_stop(adapter);

	/* Disable I2C controller, and force our state to stopped */
	temp = i2c_fsl->hwdata->i2cr_ien_opcode ^ I2CR_IEN;
	fsl_i2c_write_reg(temp, i2c_fsl, FSL_I2C_I2CR);

	return (result < 0) ? result : num;
}

static void i2c_fsl_prepare_recovery(struct i2c_adapter *adapter)
{
	int ret;

	ret = pinctrl_select_state(adapter->dev.parent, "gpio");
	if (ret)
		dev_err(adapter->dev.parent, "pinctrl failed: %s\n", strerror(-ret));
}
static void i2c_fsl_unprepare_recovery(struct i2c_adapter *adapter)
{
	int ret;

	ret = pinctrl_select_state(adapter->dev.parent, "default");
	if (ret)
		dev_err(adapter->dev.parent, "pinctrl failed: %s\n", strerror(-ret));
}

static void i2c_fsl_init_recovery(struct fsl_i2c_struct *i2c_fsl,
				  struct device *dev)
{
	if (!dev->of_node)
		return;

	i2c_fsl->rinfo.sda_gpio = of_get_named_gpio_flags(dev->of_node,
							  "sda-gpios", 0,
							  NULL);
	i2c_fsl->rinfo.scl_gpio = of_get_named_gpio_flags(dev->of_node,
							  "scl-gpios", 0,
							  NULL);

	if (!gpio_is_valid(i2c_fsl->rinfo.sda_gpio) ||
	    !gpio_is_valid(i2c_fsl->rinfo.scl_gpio))
		return;

	i2c_fsl->rinfo.get_scl = i2c_get_scl_gpio_value;
	i2c_fsl->rinfo.get_sda = i2c_get_sda_gpio_value;
	i2c_fsl->rinfo.set_scl = i2c_set_scl_gpio_value;
	i2c_fsl->rinfo.prepare_recovery = i2c_fsl_prepare_recovery;
	i2c_fsl->rinfo.unprepare_recovery = i2c_fsl_unprepare_recovery;
	i2c_fsl->rinfo.recover_bus = i2c_generic_gpio_recovery;
	i2c_fsl->adapter.bus_recovery_info = &i2c_fsl->rinfo;

	dev_dbg(dev, "initialized recovery info\n");
}

static int __init i2c_fsl_probe(struct device *pdev)
{
	struct resource *iores;
	struct fsl_i2c_struct *i2c_fsl;
	struct i2c_platform_data *pdata;
	int ret;
	int bitrate;

	pdata = pdev->platform_data;

	i2c_fsl = xzalloc(sizeof(*i2c_fsl));
	i2c_fsl->dev = pdev;

#ifdef CONFIG_COMMON_CLK
	i2c_fsl->clk = clk_get(pdev, NULL);
	if (IS_ERR(i2c_fsl->clk)) {
		ret = PTR_ERR(i2c_fsl->clk);
		goto fail;
	}
	clk_enable(i2c_fsl->clk);
#endif

	if (IS_ENABLED(CONFIG_OFDEVICE)) {
		i2c_fsl->hwdata = of_device_get_match_data(pdev);
		if (!i2c_fsl->hwdata) {
			ret = -EINVAL;
			goto fail;
		}
	} else {
		i2c_fsl->hwdata = &imx21_i2c_hwdata;
	}

	/* Setup i2c_fsl driver structure */
	i2c_fsl->adapter.master_xfer = i2c_fsl_xfer;
	i2c_fsl->adapter.nr = pdev->id;
	i2c_fsl->adapter.dev.parent = pdev;
	i2c_fsl->adapter.dev.of_node = pdev->of_node;
	iores = dev_request_mem_resource(pdev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto fail;
	}
	i2c_fsl->base = IOMEM(iores->start);

	i2c_fsl_init_recovery(i2c_fsl, pdev);

	i2c_fsl->dfsrr = -1;

	/* Set up clock divider */
	bitrate = 100000;
	of_property_read_u32(pdev->of_node, "clock-frequency", &bitrate);
	if (pdata && pdata->bitrate)
		bitrate = pdata->bitrate;

	i2c_fsl_set_clk(i2c_fsl, bitrate);

	/* Set up chip registers to defaults */
	fsl_i2c_write_reg(i2c_fsl->hwdata->i2cr_ien_opcode ^ I2CR_IEN,
			  i2c_fsl, FSL_I2C_I2CR);
	fsl_i2c_write_reg(i2c_fsl->hwdata->i2sr_clr_opcode, i2c_fsl, FSL_I2C_I2SR);

	/* Add I2C adapter */
	ret = i2c_add_numbered_adapter(&i2c_fsl->adapter);
	if (ret < 0) {
		dev_err(pdev, "registration failed\n");
		goto fail;
	}

	return 0;

fail:
	kfree(i2c_fsl);
	return ret;
}

static const struct fsl_i2c_hwdata imx21_i2c_hwdata = {
	.regshift		= IMX_I2C_REGSHIFT,
	.clk_div		= imx_i2c_clk_div,
	.ndivs			= ARRAY_SIZE(imx_i2c_clk_div),
	.i2sr_clr_opcode	= I2SR_CLR_OPCODE_W0C,
	.i2cr_ien_opcode	= I2CR_IEN_OPCODE_1,
};

static const struct fsl_i2c_hwdata vf610_i2c_hwdata = {
	.regshift		= VF610_I2C_REGSHIFT,
	.clk_div		= vf610_i2c_clk_div,
	.ndivs			= ARRAY_SIZE(vf610_i2c_clk_div),
	.i2sr_clr_opcode	= I2SR_CLR_OPCODE_W1C,
	.i2cr_ien_opcode	= I2CR_IEN_OPCODE_0,
};

static __maybe_unused struct of_device_id imx_i2c_dt_ids[] = {
	{ .compatible = "fsl,imx21-i2c", .data = &imx21_i2c_hwdata, },
	{ .compatible = "fsl,vf610-i2c", .data = &vf610_i2c_hwdata, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_i2c_dt_ids);

static struct driver i2c_fsl_driver = {
	.probe	= i2c_fsl_probe,
	.name	= "i2c-fsl",
	.of_compatible = DRV_OF_COMPAT(imx_i2c_dt_ids),
};
coredevice_platform_driver(i2c_fsl_driver);

/*
 * Copyright 2012 GE Intelligent Platforms, Inc
 * Copyright (C) 2002 Motorola GSG-China
 *               2009 Marc Kleine-Budde, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
#include <malloc.h>
#include <types.h>
#include <xfuncs.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <io.h>
#include <i2c/i2c.h>
#include <mach/clock.h>

/* This will be the driver name */
#define DRIVER_NAME "i2c-fsl"

/* Default value */
#define FSL_I2C_BIT_RATE	100000	/* 100kHz */

/* FSL I2C registers */
#define FSL_I2C_IADR	0x00	/* i2c slave address */
#define FSL_I2C_IFDR	0x04	/* i2c frequency divider */
#define FSL_I2C_I2CR	0x08	/* i2c control */
#define FSL_I2C_I2SR	0x0C	/* i2c status */
#define FSL_I2C_I2DR	0x10	/* i2c transfer data */
#define FSL_I2C_DFSRR	0x14	/* i2c digital filter sampling rate */

/* Bits of FSL I2C registers */
#define I2SR_RXAK	0x01
#define I2SR_IIF	0x02
#define I2SR_SRW	0x04
#define I2SR_IAL	0x10
#define I2SR_IBB	0x20
#define I2SR_IAAS	0x40
#define I2SR_ICF	0x80
#define I2CR_RSTA	0x04
#define I2CR_TXAK	0x08
#define I2CR_MTX	0x10
#define I2CR_MSTA	0x20
#define I2CR_IIEN	0x40
#define I2CR_IEN	0x80

/*
 * sorted list of clock divider, register value pairs
 * taken from table 26-5, p.26-9, Freescale i.MX
 * Integrated Portable System Processor Reference Manual
 * Document Number: MC9328MXLRM, Rev. 5.1, 06/2007
 *
 * Duplicated divider values removed from list
 */
#ifndef CONFIG_PPC
static u16 i2c_clk_div[50][2] = {
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
#endif

struct fsl_i2c_struct {
	void __iomem		*base;
	struct clk		*clk;
	struct i2c_adapter	adapter;
	unsigned int 		disable_delay;
	int			stopped;
	unsigned int		ifdr;	/* FSL_I2C_IFDR */
	unsigned int		dfsrr;  /* FSL_I2C_DFSRR */
};
#define to_fsl_i2c_struct(a)	container_of(a, struct fsl_i2c_struct, adapter)

#ifdef CONFIG_I2C_DEBUG
static void i2c_fsl_dump_reg(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	u32 reg_cr, reg_sr;

	reg_cr = readb(i2c_fsl->base + FSL_I2C_I2CR);
	reg_sr = readb(i2c_fsl->base + FSL_I2C_I2SR);

	dev_dbg(adapter->dev, "CONTROL:\t"
		"IEN =%d, IIEN=%d, MSTA=%d, MTX =%d, TXAK=%d, RSTA=%d\n",
		(reg_cr & I2CR_IEN  ? 1 : 0), (reg_cr & I2CR_IIEN ? 1 : 0),
		(reg_cr & I2CR_MSTA ? 1 : 0), (reg_cr & I2CR_MTX  ? 1 : 0),
		(reg_cr & I2CR_TXAK ? 1 : 0), (reg_cr & I2CR_RSTA ? 1 : 0));

	dev_dbg(adapter->dev, "STATUS:\t"
		"ICF =%d, IAAS=%d, IB  =%d, IAL =%d, SRW =%d, IIF =%d, RXAK=%d\n",
		(reg_sr & I2SR_ICF  ? 1 : 0), (reg_sr & I2SR_IAAS ? 1 : 0),
		(reg_sr & I2SR_IBB  ? 1 : 0), (reg_sr & I2SR_IAL  ? 1 : 0),
		(reg_sr & I2SR_SRW  ? 1 : 0), (reg_sr & I2SR_IIF  ? 1 : 0),
		(reg_sr & I2SR_RXAK ? 1 : 0));
}
#else
static inline void i2c_fsl_dump_reg(struct i2c_adapter *adapter)
{
	return;
}
#endif

static int i2c_fsl_bus_busy(struct i2c_adapter *adapter, int for_busy)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	uint64_t start;
	unsigned int temp;

	start = get_time_ns();
	while (1) {
		temp = readb(base + FSL_I2C_I2SR);
		if (for_busy && (temp & I2SR_IBB))
			break;
		if (!for_busy && !(temp & I2SR_IBB))
			break;
		if (is_timeout(start, 500 * MSECOND)) {
			dev_err(&adapter->dev,
				 "<%s> timeout waiting for I2C bus %s\n",
				 __func__,for_busy ? "busy" : "not busy");
			return -EIO;
		}
	}

	return 0;
}

static int i2c_fsl_trx_complete(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	uint64_t start;

	start = get_time_ns();
	while (1) {
		unsigned int reg = readb(base + FSL_I2C_I2SR);
		if (reg & I2SR_IIF)
			break;

		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(&adapter->dev, "<%s> TXR timeout\n", __func__);
			return -EIO;
		}
	}
	writeb(0, base + FSL_I2C_I2SR);

	return 0;
}

static int i2c_fsl_acked(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	uint64_t start;

	start = get_time_ns();
	while (1) {
		unsigned int reg = readb(base + FSL_I2C_I2SR);
		if (!(reg & I2SR_RXAK))
			break;

		if (is_timeout(start, MSECOND)) {
			dev_dbg(&adapter->dev, "<%s> No ACK\n", __func__);
			return -EIO;
		}
	}

	return 0;
}

static int i2c_fsl_start(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	unsigned int temp = 0;
	int result;

	writeb(i2c_fsl->ifdr, base + FSL_I2C_IFDR);
	if (i2c_fsl->dfsrr != -1)
		writeb(i2c_fsl->dfsrr, base + FSL_I2C_DFSRR);

	/* Enable I2C controller */
	writeb(0, base + FSL_I2C_I2SR);
	writeb(I2CR_IEN, base + FSL_I2C_I2CR);

	/* Wait controller to be stable */
	udelay(100);

	/* Start I2C transaction */
	temp = readb(base + FSL_I2C_I2CR);
	temp |= I2CR_MSTA;
	writeb(temp, base + FSL_I2C_I2CR);

	result = i2c_fsl_bus_busy(adapter, 1);
	if (result)
		return result;

	i2c_fsl->stopped = 0;

	temp |= I2CR_MTX | I2CR_TXAK;
	writeb(temp, base + FSL_I2C_I2CR);

	return result;
}

static void i2c_fsl_stop(struct i2c_adapter *adapter)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	unsigned int temp = 0;

	if (!i2c_fsl->stopped) {
		/* Stop I2C transaction */
		temp = readb(base + FSL_I2C_I2CR);
		temp &= ~(I2CR_MSTA | I2CR_MTX);
		writeb(temp, base + FSL_I2C_I2CR);
		/* wait for the stop condition to be send, otherwise the i2c
		 * controller is disabled before the STOP is sent completely */
		i2c_fsl->stopped = i2c_fsl_bus_busy(adapter, 0) ? 0 : 1;
	}

	if (!i2c_fsl->stopped) {
		i2c_fsl_bus_busy(adapter, 0);
		i2c_fsl->stopped = 1;
	}

	/* Disable I2C controller, and force our state to stopped */
	writeb(0, base + FSL_I2C_I2CR);
}

#ifdef CONFIG_PPC
static void i2c_fsl_set_clk(struct fsl_i2c_struct *i2c_fsl,
				    unsigned int rate)
{
	void __iomem *base;
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
	base = i2c_fsl->base;
	i2c_clk = fsl_get_i2c_freq();
	divider = min((unsigned short)(i2c_clk / rate), (unsigned short) -1);

	/*
	 * Condition 1: dfsr <= 50ns/T (T=period of I2C source clock in ns).
	 * or (dfsr * T) <= 50ns.
	 * Translate to dfsr = 5 * Frequency / 100,000,000
	 */
	dfsr = (5 * (i2c_clk / 1000)) / 100000;
	dev_dbg(&i2c_fsl->adapter.dev,
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
				dev_dbg(&i2c_fsl->adapter.dev,
					"FDR:0x%.2x, div:%ld, ga:0x%x, gb:0x%x,"
					" a:%d, b:%d, speed:%d\n", fdr, est_div,
					ga, gb, a, b, rate);
				/* Condition 2 not accounted for */
				dev_dbg(&i2c_fsl->adapter.dev,
					"Tr <= %d ns\n", (b - 3 * dfsr) *
					1000000 / (i2c_clk / 1000));
			}
		}
		if (a == 20)
			a += 2;
		if (a == 24)
			a += 4;
	}
	dev_dbg(&i2c_fsl->adapter.dev,
		"divider:%d, est_div:%ld, DFSR:%d\n", divider, est_div, dfsr);
	dev_dbg(&i2c_fsl->adapter.dev, "FDR:0x%.2x, speed:%d\n", fdr, rate);
	i2c_fsl->ifdr = fdr;
	i2c_fsl->dfsrr = dfsr;
}
#else
static void i2c_fsl_set_clk(struct fsl_i2c_struct *i2c_fsl,
			    unsigned int rate)
{
	unsigned int i2c_clk_rate;
	unsigned int div;
	int i;

	/* Divider value calculation */
	i2c_clk_rate = clk_get_rate(i2c_fsl->clk);
	div = (i2c_clk_rate + rate - 1) / rate;
	if (div < i2c_clk_div[0][0])
		i = 0;
	else if (div > i2c_clk_div[ARRAY_SIZE(i2c_clk_div) - 1][0])
		i = ARRAY_SIZE(i2c_clk_div) - 1;
	else
		for (i = 0; i2c_clk_div[i][0] < div; i++)
			;

	/* Store divider value */
	i2c_fsl->ifdr = i2c_clk_div[i][1];

	/*
	 * There dummy delay is calculated.
	 * It should be about one I2C clock period long.
	 * This delay is used in I2C bus disable function
	 * to fix chip hardware bug.
	 */
	i2c_fsl->disable_delay =
		(500000U * i2c_clk_div[i][0] + (i2c_clk_rate / 2) - 1) /
		(i2c_clk_rate / 2);

	dev_dbg(&i2c_fsl->adapter.dev, "<%s> I2C_CLK=%d, REQ DIV=%d\n",
		__func__, i2c_clk_rate, div);
	dev_dbg(&i2c_fsl->adapter.dev, "<%s> IFDR[IC]=0x%x, REAL DIV=%d\n",
		__func__, i2c_clk_div[i][1], i2c_clk_div[i][0]);
}
#endif

static int i2c_fsl_write(struct i2c_adapter *adapter, struct i2c_msg *msgs)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	int i, result;

	if ( !(msgs->flags & I2C_M_DATA_ONLY) ) {
		dev_dbg(&adapter->dev,
			"<%s> write slave address: addr=0x%02x\n",
			__func__, msgs->addr << 1);

		/* write slave address */
		writeb(msgs->addr << 1, base + FSL_I2C_I2DR);

		result = i2c_fsl_trx_complete(adapter);
		if (result)
			return result;
		result = i2c_fsl_acked(adapter);
		if (result)
			return result;
	}

	/* write data */
	for (i = 0; i < msgs->len; i++) {
		dev_dbg(&adapter->dev,
			"<%s> write byte: B%d=0x%02X\n",
			__func__, i, msgs->buf[i]);
		writeb(msgs->buf[i], base + FSL_I2C_I2DR);

		result = i2c_fsl_trx_complete(adapter);
		if (result)
			return result;
		result = i2c_fsl_acked(adapter);
		if (result)
			return result;
	}
	return 0;
}

static int i2c_fsl_read(struct i2c_adapter *adapter, struct i2c_msg *msgs)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	int i, result;
	unsigned int temp;

	/* clear IIF */
	writeb(0x0, base + FSL_I2C_I2SR);

	if ( !(msgs->flags & I2C_M_DATA_ONLY) ) {
		dev_dbg(&adapter->dev,
			"<%s> write slave address: addr=0x%02x\n",
			__func__, (msgs->addr << 1) | 0x01);

		/* write slave address */
		writeb((msgs->addr << 1) | 0x01, base + FSL_I2C_I2DR);

		result = i2c_fsl_trx_complete(adapter);
		if (result)
			return result;
		result = i2c_fsl_acked(adapter);
		if (result)
			return result;
	}

	/* setup bus to read data */
	temp = readb(base + FSL_I2C_I2CR);
	temp &= ~I2CR_MTX;
	if (msgs->len - 1)
		temp &= ~I2CR_TXAK;
	writeb(temp, base + FSL_I2C_I2CR);

	readb(base + FSL_I2C_I2DR);	/* dummy read */

	/* read data */
	for (i = 0; i < msgs->len; i++) {
		result = i2c_fsl_trx_complete(adapter);
		if (result)
			return result;

		if (i == (msgs->len - 1)) {
			/*
			 * It must generate STOP before read I2DR to prevent
			 * controller from generating another clock cycle
			 */
			temp = readb(base + FSL_I2C_I2CR);
			temp &= ~(I2CR_MSTA | I2CR_MTX);
			writeb(temp, base + FSL_I2C_I2CR);

			/*
			 * adding this delay helps on low bitrates
			 */
			udelay(i2c_fsl->disable_delay);

			i2c_fsl_bus_busy(adapter, 0);
			i2c_fsl->stopped = 1;
		} else if (i == (msgs->len - 2)) {
			temp = readb(base + FSL_I2C_I2CR);
			temp |= I2CR_TXAK;
			writeb(temp, base + FSL_I2C_I2CR);
		}
		msgs->buf[i] = readb(base + FSL_I2C_I2DR);

		dev_dbg(&adapter->dev, "<%s> read byte: B%d=0x%02X\n",
			__func__, i, msgs->buf[i]);
	}
	return 0;
}

static int i2c_fsl_xfer(struct i2c_adapter *adapter,
			struct i2c_msg *msgs, int num)
{
	struct fsl_i2c_struct *i2c_fsl = to_fsl_i2c_struct(adapter);
	void __iomem *base = i2c_fsl->base;
	unsigned int i, temp;
	int result;

	/* Start I2C transfer */
	result = i2c_fsl_start(adapter);
	if (result)
		goto fail0;

	/* read/write data */
	for (i = 0; i < num; i++) {
		if (i && !(msgs[i].flags & I2C_M_DATA_ONLY)) {
			temp = readb(base + FSL_I2C_I2CR);
			temp |= I2CR_RSTA;
			writeb(temp, base + FSL_I2C_I2CR);

			result = i2c_fsl_bus_busy(adapter, 1);
			if (result)
				goto fail0;
		}
		i2c_fsl_dump_reg(adapter);

		/* write/read data */
		if (msgs[i].flags & I2C_M_RD)
			result = i2c_fsl_read(adapter, &msgs[i]);
		else
			result = i2c_fsl_write(adapter, &msgs[i]);
		if (result)
			goto fail0;
	}

fail0:
	/* Stop I2C transfer */
	i2c_fsl_stop(adapter);

	return (result < 0) ? result : num;
}

static int __init i2c_fsl_probe(struct device_d *pdev)
{
	struct fsl_i2c_struct *i2c_fsl;
	struct i2c_platform_data *pdata;
	int ret;

	pdata = pdev->platform_data;

	i2c_fsl = kzalloc(sizeof(struct fsl_i2c_struct), GFP_KERNEL);

#ifdef CONFIG_COMMON_CLK
	i2c_fsl->clk = clk_get(pdev, NULL);
	if (IS_ERR(i2c_fsl->clk)) {
		ret = PTR_ERR(i2c_fsl->clk);
		goto fail;
	}
#endif
	/* Setup i2c_fsl driver structure */
	i2c_fsl->adapter.master_xfer = i2c_fsl_xfer;
	i2c_fsl->adapter.nr = pdev->id;
	i2c_fsl->adapter.dev.parent = pdev;
	i2c_fsl->adapter.dev.device_node = pdev->device_node;
	i2c_fsl->base = dev_request_mem_region(pdev, 0);
	if (IS_ERR(i2c_fsl->base)) {
		ret = PTR_ERR(i2c_fsl->base);
		goto fail;
	}

	i2c_fsl->dfsrr = -1;

	/* Set up clock divider */
	if (pdata && pdata->bitrate)
		i2c_fsl_set_clk(i2c_fsl, pdata->bitrate);
	else
		i2c_fsl_set_clk(i2c_fsl, FSL_I2C_BIT_RATE);

	/* Set up chip registers to defaults */
	writeb(0, i2c_fsl->base + FSL_I2C_I2CR);
	writeb(0, i2c_fsl->base + FSL_I2C_I2SR);

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

static __maybe_unused struct of_device_id imx_i2c_dt_ids[] = {
	{
		.compatible = "fsl,imx21-i2c",
	}, {
		/* sentinel */
	}
};

static struct driver_d i2c_fsl_driver = {
	.probe	= i2c_fsl_probe,
	.name	= DRIVER_NAME,
	.of_compatible = DRV_OF_COMPAT(imx_i2c_dt_ids),
};
coredevice_platform_driver(i2c_fsl_driver);

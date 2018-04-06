/*
 * ocotp.c - barebox driver for the On-Chip One Time Programmable for MXS
 *
 * Copyright (C) 2012 by Wolfram Sang, Pengutronix e.K.
 * based on the kernel driver which is
 * Copyright 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <param.h>
#include <fcntl.h>
#include <malloc.h>
#include <io.h>
#include <stmp-device.h>
#include <clock.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <mach/generic.h>
#include <mach/ocotp.h>
#include <mach/power.h>

#define DRIVERNAME "ocotp"

#define OCOTP_CTRL			0x0
#  define OCOTP_CTRL_ADDR_MASK		0x3f
#  define OCOTP_CTRL_BUSY		(1 << 8)
#  define OCOTP_CTRL_ERROR		(1 << 9)
#  define OCOTP_CTRL_RD_BANK_OPEN	(1 << 12)
#  define OCOTP_CTRL_WR_UNLOCK		0x3e770000

#define OCOTP_DATA			0x10

#define OCOTP_WORD_OFFSET		0x20

struct ocotp_priv {
	struct device_d dev;
	struct cdev cdev;
	void __iomem *base;
	unsigned int write_enable;
	struct clk *clk;
};

static int mxs_ocotp_wait_busy(struct ocotp_priv *priv)
{
	uint64_t start = get_time_ns();

	/* check both BUSY and ERROR cleared */
	while (readl(priv->base + OCOTP_CTRL) & (OCOTP_CTRL_BUSY | OCOTP_CTRL_ERROR))
		if (is_timeout(start, MSECOND))
			return -ETIMEDOUT;

	return 0;
}

static ssize_t mxs_ocotp_cdev_read(struct cdev *cdev, void *buf, size_t count,
		loff_t offset, ulong flags)
{
	struct ocotp_priv *priv = cdev->priv;
	void __iomem *base = priv->base;
	size_t size = min((loff_t)count, cdev->size - offset);
	int i;

	/*
	 * clk_enable(hbus_clk) for ocotp can be skipped
	 * as it must be on when system is running.
	 */

	/* try to clear ERROR bit */
	writel(OCOTP_CTRL_ERROR, base + OCOTP_CTRL + STMP_OFFSET_REG_CLR);

	if (mxs_ocotp_wait_busy(priv))
		return -ETIMEDOUT;

	/* open OCOTP banks for read */
	writel(OCOTP_CTRL_RD_BANK_OPEN, base + OCOTP_CTRL + STMP_OFFSET_REG_SET);

	/* approximately wait 32 hclk cycles */
	udelay(1);

	/* poll BUSY bit becoming cleared */
	if (mxs_ocotp_wait_busy(priv))
		return -ETIMEDOUT;

	for (i = 0; i < size; i++)
		/* When reading bytewise, we need to hop over the SET/CLR/TGL regs */
		((u8 *)buf)[i] = readb(base + OCOTP_WORD_OFFSET +
				(((i + offset) & 0xfc) << 2) + ((i + offset) & 3));

	/* close banks for power saving */
	writel(OCOTP_CTRL_RD_BANK_OPEN, base + OCOTP_CTRL + STMP_OFFSET_REG_CLR);

	return size;
}

static ssize_t mxs_ocotp_cdev_write(struct cdev *cdev, const void *buf, size_t count,
		loff_t offset, ulong flags)
{
	struct ocotp_priv *priv = cdev->priv;
	void __iomem *base = priv->base;
	unsigned long old_hclk, aligned_offset;
	int old_vddio, num_words, num_bytes, i, ret = 0;
	u8 *work_buf;
	u32 reg;

	if (!priv->write_enable)
		return -EPERM;

	/* we can only work on u32, so calc some helpers */
	aligned_offset = offset & ~3UL;
	num_words = DIV_ROUND_UP(offset - aligned_offset + count, 4);
	num_bytes = num_words << 2;

	/* read in all words which will be modified */
	work_buf = xmalloc(num_bytes);

	i = mxs_ocotp_cdev_read(cdev, work_buf, num_bytes, aligned_offset, 0);
	if (i != num_bytes) {
		ret = -ENXIO;
		goto free_mem;
	}

	/* modify read words with to be written data */
	for (i = 0; i < count; i++)
		work_buf[offset - aligned_offset + i] |= ((u8 *)buf)[i];

	/* prepare system for OTP write */
	old_hclk = clk_get_rate(priv->clk);
	old_vddio = imx_get_vddio();

	clk_set_rate(priv->clk, 24000000);
	imx_set_vddio(2800000);

	writel(OCOTP_CTRL_RD_BANK_OPEN, base + OCOTP_CTRL + STMP_OFFSET_REG_CLR);

	if (mxs_ocotp_wait_busy(priv)) {
		ret = -ETIMEDOUT;
		goto restore_system;
	}

	/* write word for word via data register */
	for (i = 0; i < num_words; i++) {
		reg = readl(base + OCOTP_CTRL) & ~OCOTP_CTRL_ADDR_MASK;
		reg |= OCOTP_CTRL_WR_UNLOCK | ((aligned_offset >> 2) + i);
		writel(reg, base + OCOTP_CTRL);

		writel(((u32 *)work_buf)[i], base + OCOTP_DATA);

		if (mxs_ocotp_wait_busy(priv)) {
			ret = -ETIMEDOUT;
			goto restore_system;
		}

		mdelay(2);
	}

restore_system:
	imx_set_vddio(old_vddio);
	clk_set_rate(priv->clk, old_hclk);
free_mem:
	free(work_buf);

	return ret;
}

static struct cdev_operations mxs_ocotp_ops = {
	.read	= mxs_ocotp_cdev_read,
	.lseek	= dev_lseek_default,
};

static int mxs_ocotp_probe(struct device_d *dev)
{
	struct resource *iores;
	int err;
	struct ocotp_priv *priv = xzalloc(sizeof (*priv));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->base = IOMEM(iores->start);

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);
	priv->cdev.dev = dev;
	priv->cdev.ops = &mxs_ocotp_ops;
	priv->cdev.priv = priv;
	priv->cdev.size = cpu_is_mx23() ? 128 : 160;
	priv->cdev.name = DRIVERNAME;

	strcpy(priv->dev.name, "ocotp");
	priv->dev.parent = dev;
	err = register_device(&priv->dev);
	if (err)
		return err;

	err = devfs_create(&priv->cdev);
	if (err < 0)
		return err;

	if (IS_ENABLED(CONFIG_MXS_OCOTP_WRITABLE)) {
		mxs_ocotp_ops.write = mxs_ocotp_cdev_write;
		dev_add_param_bool(&priv->dev, "permanent_write_enable",
			NULL, NULL, &priv->write_enable, NULL);
	}

	return 0;
}

static __maybe_unused struct of_device_id mxs_ocotp_compatible[] = {
	{
		.compatible = "fsl,ocotp",
	}, {
		/* sentinel */
	}
};

static struct driver_d mxs_ocotp_driver = {
	.name	= DRIVERNAME,
	.probe	= mxs_ocotp_probe,
	.of_compatible = DRV_OF_COMPAT(mxs_ocotp_compatible),
};

static int mxs_ocotp_init(void)
{
	platform_driver_register(&mxs_ocotp_driver);

	return 0;
}
coredevice_initcall(mxs_ocotp_init);

int mxs_ocotp_read(void *buf, int count, int offset)
{
	struct cdev *cdev;
	int ret;

	cdev = cdev_open(DRIVERNAME, O_RDONLY);
	if (!cdev)
		return -ENODEV;

	ret = cdev_read(cdev, buf, count, offset, 0);

	cdev_close(cdev);

	return ret;
}

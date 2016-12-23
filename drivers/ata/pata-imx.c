/*
 * Copyright (C) 2011 Juergen Beisert, Pengutronix
 * Copyright (C) 2012 Sascha Hauer, Pengutronix
 *
 * Derived from the Linux kernel: Generic platform device PATA driver
 *  Copyright (C) 2006 - 2007  Paul Mundt
 *  Based on pata_pcmcia:
 *  Copyright 2005-2006 Red Hat Inc, all rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
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
 */

#include <common.h>
#include <init.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <ata_drive.h>
#include <platform_data/ide.h>
#include <io.h>
#include <of.h>
#include <linux/err.h>
#include <linux/clk.h>

#define PATA_IMX_ATA_TIME_OFF		0x0
#define PATA_IMX_ATA_TIME_ON		0x1
#define PATA_IMX_ATA_TIME_1		0x2
#define PATA_IMX_ATA_TIME_2W		0x3
#define PATA_IMX_ATA_TIME_2R		0x4
#define PATA_IMX_ATA_TIME_AX		0x5
#define PATA_IMX_ATA_TIME_PIO_RDX	0x6
#define PATA_IMX_ATA_TIME_4		0x7
#define PATA_IMX_ATA_TIME_9		0x8
#define PATA_IMX_ATA_TIME_M		0x9
#define PATA_IMX_ATA_TIME_JN		0xa
#define PATA_IMX_ATA_TIME_D		0xb
#define PATA_IMX_ATA_TIME_K		0xc
#define PATA_IMX_ATA_TIME_ACK		0xd
#define PATA_IMX_ATA_TIME_ENV		0xe
#define PATA_IMX_ATA_TIME_UDMA_RDX	0xf
#define PATA_IMX_ATA_TIME_ZAH		0x10
#define PATA_IMX_ATA_TIME_MLIX		0x11
#define PATA_IMX_ATA_TIME_DVH		0x12
#define PATA_IMX_ATA_TIME_DZFS		0x13
#define PATA_IMX_ATA_TIME_DVS		0x14
#define PATA_IMX_ATA_TIME_CVH		0x15
#define PATA_IMX_ATA_TIME_SS		0x16
#define PATA_IMX_ATA_TIME_CYC		0x17
#define PATA_IMX_ATA_CONTROL		0x24
#define PATA_IMX_ATA_CTRL_FIFO_RST_B	(1<<7)
#define PATA_IMX_ATA_CTRL_ATA_RST_B	(1<<6)
#define PATA_IMX_ATA_CTRL_IORDY_EN	(1<<0)
#define PATA_IMX_ATA_INT_EN		0x2C
#define PATA_IMX_ATA_INTR_ATA_INTRQ2	(1<<3)
#define PATA_IMX_DRIVE_DATA		0xA0
#define PATA_IMX_DRIVE_CONTROL		0xD8

static uint16_t pio_t1[]    = { 70,  50,  30,  30,  25 };
static uint16_t pio_t2_8[]  = { 290, 290, 290, 80,  70 };
static uint16_t pio_t4[]    = { 30,  20,  15,  10,  10 };
static uint16_t pio_t9[]    = { 20,  15,  10,  10,  10 };
static uint16_t pio_tA[]    = { 50,  50,  50,  50,  50 };

static void pata_imx_set_bus_timing(void __iomem *base, unsigned long clkrate,
		unsigned char mode)
{
	uint32_t T = 1000000000 / clkrate;

	if (mode >= ARRAY_SIZE(pio_t1))
		return;

	/* Write TIME_OFF/ON/1/2W */
	writeb(3,  base + PATA_IMX_ATA_TIME_OFF);
	writeb(3,  base + PATA_IMX_ATA_TIME_ON);
	writeb((pio_t1[mode] + T) / T, base + PATA_IMX_ATA_TIME_1);
	writeb((pio_t2_8[mode] + T) / T, base + PATA_IMX_ATA_TIME_2W);

	/* Write TIME_2R/AX/RDX/4 */
	writeb((pio_t2_8[mode] + T) / T,  base + PATA_IMX_ATA_TIME_2R);
	writeb((pio_tA[mode] + T) / T + 2,  base + PATA_IMX_ATA_TIME_AX);
	writeb(1,  base + PATA_IMX_ATA_TIME_PIO_RDX);
	writeb((pio_t4[mode] + T) / T,  base + PATA_IMX_ATA_TIME_4);

	/* Write TIME_9 ; the rest of timing registers is irrelevant for PIO */
	writeb((pio_t9[mode] + T) / T,  base + PATA_IMX_ATA_TIME_9);
}

/**
 * Setup the register specific addresses for an ATA like divice
 * @param reg_base Base address of the standard ATA like registers
 * @param alt_base Base address of the alternate ATA like registers
 * @param ioaddr Register file structure to setup
 * @param shift Shift offset between registers
 *
 * Some of the registers have different names but use the same offset. This is
 * due to the fact read or write access at the same offset reaches different
 * registers.
 */
static void imx_pata_setup_port(void *reg_base, void *alt_base,
			struct ata_ioports *ioaddr, unsigned shift)
{
	/* standard registers (0 ... 7) */
	ioaddr->cmd_addr = reg_base;

	ioaddr->data_addr = reg_base + (IDE_REG_DATA << shift);

	ioaddr->error_addr = reg_base + (IDE_REG_ERR << shift);
	ioaddr->feature_addr = reg_base + (IDE_REG_FEATURE << shift);

	ioaddr->nsect_addr = reg_base + (IDE_REG_NSECT << shift);

	ioaddr->lbal_addr = reg_base + (IDE_REG_LBAL << shift);

	ioaddr->lbam_addr = reg_base + (IDE_REG_LBAM << shift);

	ioaddr->lbah_addr = reg_base + (IDE_REG_LBAH << shift);

	ioaddr->device_addr = reg_base + (IDE_REG_DEVICE << shift);

	ioaddr->status_addr = reg_base + (IDE_REG_STATUS << shift);
	ioaddr->command_addr = reg_base + (IDE_REG_CMD << shift);

	if (alt_base) {
		ioaddr->altstatus_addr = alt_base + (IDE_REG_ALT_STATUS << shift);
		ioaddr->ctl_addr = alt_base + (IDE_REG_DEV_CTL << shift);

		ioaddr->alt_dev_addr = alt_base + (IDE_REG_DRV_ADDR << shift);
	} else {
		ioaddr->altstatus_addr = ioaddr->status_addr;
		ioaddr->ctl_addr = ioaddr->status_addr;
		/* ioaddr->alt_dev_addr not used in driver */
	}
}

static int pata_imx_detect(struct device_d *dev)
{
	struct ide_port *ide = dev->priv;

	return ata_port_detect(&ide->port);
}

static int imx_pata_probe(struct device_d *dev)
{
	struct resource *iores;
	struct ide_port *ide;
	struct clk *clk;
	void __iomem *base;
	int ret;

	ide = xzalloc(sizeof(*ide));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto out_free;
	}

	imx_pata_setup_port(base + PATA_IMX_DRIVE_DATA,
			base + PATA_IMX_DRIVE_CONTROL, &ide->io, 2);

	/* deassert resets */
	writel(PATA_IMX_ATA_CTRL_FIFO_RST_B |
			PATA_IMX_ATA_CTRL_ATA_RST_B,
			base + PATA_IMX_ATA_CONTROL);

	pata_imx_set_bus_timing(base, clk_get_rate(clk), 4);


	ide->port.dev = dev;
	ide->port.devname = xstrdup(of_alias_get(dev->device_node));

	dev->priv = ide;
	dev->detect = pata_imx_detect;

	ret = ide_port_register(ide);
	if (ret) {
		dev_err(dev, "Cannot register IDE interface: %s\n",
				strerror(-ret));
		goto out_free_clk;
	}

	return 0;

out_free_clk:
	clk_put(clk);

out_free:
	free(ide);

	return ret;
}

static __maybe_unused struct of_device_id imx_pata_dt_ids[] = {
	{
		.compatible = "fsl,imx27-pata",
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_pata_driver = {
	.name   = "imx-pata",
	.probe  = imx_pata_probe,
	.of_compatible = DRV_OF_COMPAT(imx_pata_dt_ids),
};
device_platform_driver(imx_pata_driver);

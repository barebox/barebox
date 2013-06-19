/*
 * Copyright (C) 2011 Juergen Beisert, Pengutronix
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
#include <platform_ide.h>

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
static void platform_ide_setup_port(void *reg_base, void *alt_base,
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

static int platform_ide_probe(struct device_d *dev)
{
	int rc;
	struct ide_port_info *pdata = dev->platform_data;
	struct ide_port *ide;
	void *reg_base, *alt_base;

	if (pdata == NULL) {
		dev_err(dev, "No platform data. Cannot continue\n");
		return -EINVAL;
	}

	ide = xzalloc(sizeof(*ide));
	reg_base = dev_request_mem_region(dev, 0);
	alt_base = dev_request_mem_region(dev, 1);
	platform_ide_setup_port(reg_base, alt_base, &ide->io, pdata->ioport_shift);
	ide->io.reset = pdata->reset;
	ide->io.dataif_be = pdata->dataif_be;

	rc = ide_port_register(ide);
	if (rc != 0) {
		dev_err(dev, "Cannot register IDE interface\n");
		free(ide);
	}

	return rc;
}

static struct driver_d platform_ide_driver = {
	.name   = "ide_intf",
	.probe  = platform_ide_probe,
};
device_platform_driver(platform_ide_driver);

/**
 * @file
 * @brief Interface driver for an ATA drive behind an IDE like interface
 *
 * This communication driver does all accesses to the drive via memory IO
 * instructions.
 *
 * This kind of interface is also useable for regular bus like access, when
 * there is no dedicated IDE available.
 *
 * "IDE like" means the platform data defines addresses to the register file
 * of the attached ATA drive.
 *
 * This driver does not change any access timings due to the fact it has no idea
 * how to do so. So, do not expect an impressive data throughput.
 *
 * @todo Support also the IO port access method, the x86 architecture is using
 */

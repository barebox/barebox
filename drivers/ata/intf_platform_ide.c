// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011 Juergen Beisert, Pengutronix
 *
 * Derived from the Linux kernel: Generic platform device PATA driver
 *  Copyright (C) 2006 - 2007  Paul Mundt
 *  Based on pata_pcmcia:
 *  Copyright 2005-2006 Red Hat Inc, all rights reserved.
 */

#include <common.h>
#include <init.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <ata_drive.h>
#include <platform_data/ide.h>
#include <linux/err.h>
#include <of.h>

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

static int platform_ide_probe(struct device *dev)
{
	struct resource *iores;
	int rc;
	struct ide_port_info *pdata = dev->platform_data;
	struct ide_port *ide;
	void *reg_base, *alt_base = NULL;
	struct resource *reg, *alt;
	int mmio = 0;
	struct device_node *dn = dev->of_node;
	u32 ioport_shift = 0;
	int dataif_be = 0;
	void (*reset)(int) = NULL;

	if (pdata) {
		ioport_shift = pdata->ioport_shift;
		dataif_be = pdata->dataif_be;
		reset = pdata->reset;
	} else if (dn) {
		of_property_read_u32(dn, "reg-shift", &ioport_shift);
	} else {
		dev_err(dev, "No platform data. Cannot continue\n");
		return -EINVAL;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (!IS_ERR(iores)) {
		reg_base = IOMEM(iores->start);
		mmio = 1;
		iores = dev_request_mem_resource(dev, 1);
		if (!IS_ERR(iores))
			alt_base = IOMEM(iores->start);
		else
			alt_base = NULL;
	} else {
		reg = dev_get_resource(dev, IORESOURCE_IO, 0);
		if (IS_ERR(reg))
			return PTR_ERR(reg);

		reg = request_ioport_region(dev_name(dev), reg->start,
					    reg->end);
		if (!reg)
			return -ENODEV;

		reg_base = (void __force __iomem *) reg->start;

		alt = dev_get_resource(dev, IORESOURCE_IO, 1);
		if (!IS_ERR(alt)) {
			alt = request_ioport_region(dev_name(dev),
						    alt->start,
						    alt->end);
			if (!alt)
				return -ENODEV;
		}
	}

	ide = xzalloc(sizeof(*ide));
	ide->io.mmio = mmio;

	platform_ide_setup_port(reg_base, alt_base, &ide->io, ioport_shift);
	ide->io.reset = reset;
	ide->io.dataif_be = dataif_be;

	rc = ide_port_register(ide);
	if (rc != 0) {
		dev_err(dev, "Cannot register IDE interface\n");
		free(ide);
	}

	return rc;
}

static __maybe_unused struct of_device_id platform_ide_dt_ids[] = {
	{
		.compatible = "ata-generic",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, platform_ide_dt_ids);

static struct driver platform_ide_driver = {
	.name   = "ide_intf",
	.probe  = platform_ide_probe,
	.of_compatible = DRV_OF_COMPAT(platform_ide_dt_ids),
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
 */

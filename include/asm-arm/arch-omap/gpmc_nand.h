/**
 * @file
 * @brief This file contains exported structure for NAND
 *
 * FileName: include/asm-arm/arch-omap/gpmc_nand.h
 *
 * OMAP's General Purpose Memory Controller (GPMC) has a NAND controller
 * embedded. this file provides the platform data structure required to
 * hook on to it.
 *
 */
/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * Originally from Linux kernel:
 * http://linux.omap.com/pub/kernel/3430zoom/linux-ldp-v1.3.tar.gz
 * include/asm-arm/arch-omap/nand.h
 *
 * Copyright (C) 2006 Micron Technology Inc.
 * Author: Shahrom Sharif-Kashani
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_OMAP_NAND_GPMC_H
#define  __ASM_OMAP_NAND_GPMC_H

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>

/** omap nand platform data structure */
struct gpmc_nand_platform_data {
	/** Chip select you want to use */
	int cs;
	struct mtd_partition *parts;
	int nr_parts;
	/** If there are any special setups you'd want to do */
	int (*nand_setup) (struct gpmc_nand_platform_data *);

	/** set up if we want H/w ECC here and other
	 * platform specific configs here
	 */
	unsigned short plat_options;
	/** setup any special options */
	unsigned int options;
	/** set up device access as 8,16 as per GPMC config */
	char device_width;
	/** Set this to WAITx+1, so GPMC WAIT0 will be 1 and so on. */
	char wait_mon_pin;
	/** Set this to the max timeout for the device */
	uint64_t max_timeout;

	/* if you like a custom oob use this. */
	struct nand_ecclayout *oob;
	/** platform specific private data */
	void *priv;
};

/** Platform specific options definitions */
/** plat_options: Wait montioring pin low */
#define NAND_WAITPOL_LOW        (0 << 0)
/** plat_options: Wait montioring pin high */
#define NAND_WAITPOL_HIGH       (1 << 0)
#define NAND_WAITPOL_MASK       (1 << 0)

#ifdef CONFIG_NAND_OMAP_GPMC_HWECC
/** plat_options: hw ecc enabled */
#define NAND_HWECC_ENABLE       (1 << 1)
#endif
/** plat_options: hw ecc disabled */
#define NAND_HWECC_DISABLE      (0 << 1)
#define NAND_HWECC_MASK         (1 << 1)

/* Typical BOOTROM oob layouts-requires hwecc **/
#ifdef CONFIG_NAND_OMAP_GPMC_HWECC
/** Large Page x8 NAND device Layout */
#define GPMC_NAND_ECC_LP_x8_LAYOUT {\
	.eccbytes = 12,\
	.eccpos = {1, 2, 3, 4, 5, 6, 7, 8,\
		9, 10, 11, 12},\
	.oobfree = {\
		{.offset = 60,\
		 .length = 2 } } \
}

/** Large Page x16 NAND device Layout */
#define GPMC_NAND_ECC_LP_x16_LAYOUT {\
	.eccbytes = 12,\
	.eccpos = {2, 3, 4, 5, 6, 7, 8, 9,\
		10, 11, 12, 13},\
	.oobfree = {\
		{.offset = 60,\
		 .length = 2 } } \
}

/** Small Page x8 NAND device Layout */
#define GPMC_NAND_ECC_SP_x8_LAYOUT {\
	.eccbytes = 3,\
	.eccpos = {1, 2, 3},\
	.oobfree = {\
		{.offset = 14,\
		 .length = 2 } } \
}

/** Small Page x16 NAND device Layout */
#define GPMC_NAND_ECC_SP_x16_LAYOUT {\
	.eccbytes = 3,\
	.eccpos = {2, 3, 4},\
	.oobfree = {\
		{.offset = 14,\
		 .length = 2 } } \
}

#endif				/* CONFIG_NAND_OMAP_GPMC_HWECC */

#endif				/* __ASM_OMAP_NAND_GPMC_H */

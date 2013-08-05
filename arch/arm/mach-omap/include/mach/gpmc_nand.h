/**
 * @file
 * @brief This file contains exported structure for NAND
 *
 * OMAP's General Purpose Memory Controller (GPMC) has a NAND controller
 * embedded. this file provides the platform data structure required to
 * hook on to it.
 *
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

enum gpmc_ecc_mode {
	OMAP_ECC_SOFT,
	OMAP_ECC_HAMMING_CODE_HW_ROMCODE,
	OMAP_ECC_BCH4_CODE_HW,
	OMAP_ECC_BCH8_CODE_HW,
	OMAP_ECC_BCH8_CODE_HW_ROMCODE,
};

/** omap nand platform data structure */
struct gpmc_nand_platform_data {
	/** Chip select you want to use */
	int cs;
	struct mtd_partition *parts;
	int nr_parts;
	/** If there are any special setups you'd want to do */
	int (*nand_setup) (struct gpmc_nand_platform_data *);

	/** ecc mode to use */
	enum gpmc_ecc_mode ecc_mode;
	/** setup any special options */
	unsigned int options;
	/** set up device access as 8,16 as per GPMC config */
	char device_width;
	/** Set this to WAITx+1, so GPMC WAIT0 will be 1 and so on. */
	char wait_mon_pin;

	/* if you like a custom oob use this. */
	struct nand_ecclayout *oob;
	/** gpmc config for nand */
	struct gpmc_config *nand_cfg;
};

int omap_add_gpmc_nand_device(struct gpmc_nand_platform_data *pdata);

extern struct gpmc_config omap3_nand_cfg;
extern struct gpmc_config omap4_nand_cfg;
extern struct gpmc_config am33xx_nand_cfg;

#endif				/* __ASM_OMAP_NAND_GPMC_H */

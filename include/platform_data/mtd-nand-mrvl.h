/*
 * Copyright (C) 2014 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Taken from linux kernel mostly.
 */
#ifndef __MRVL_NAND_H
#define __MRVL_NAND_H

struct mrvl_nand_timing {
	uint16_t	id;   /* NAND id code (READID) */
	unsigned int	tCH;  /* Enable signal hold time */
	unsigned int	tCS;  /* Enable signal setup time */
	unsigned int	tWH;  /* ND_nWE high duration */
	unsigned int	tWP;  /* ND_nWE pulse time */
	unsigned int	tRH;  /* ND_nRE high duration */
	unsigned int	tRP;  /* ND_nRE pulse width */
	unsigned int	tR;   /* ND_nWE high to ND_nRE low for read */
	unsigned int	tWHR; /* ND_nWE high to ND_nRE low for status read */
	unsigned int	tAR;  /* ND_ALE low to ND_nRE low delay */
};

struct mrvl_nand_flash {
	char		*name;
	uint32_t	chip_id;
	unsigned int	page_per_block; /* Pages per block (PG_PER_BLK) */
	unsigned int	page_size;	/* Page size in bytes (PAGE_SZ) */
	unsigned int	flash_width;	/* Flash memory width (DWIDTH_M) */
	unsigned int	dfc_width;	/* Flash controller width (DWIDTH_C) */
	unsigned int	num_blocks;	/* Number of physical blocks in Flash */

	struct mrvl_nand_timing *timing;	/* NAND Flash timing */
};

/*
 * Current pxa3xx_nand controller has two chip select which
 * both be workable.
 *
 * Notice should be taken that:
 * When you want to use this feature, you should not enable the
 * keep configuration feature, for two chip select could be
 * attached with different nand chip. The different page size
 * and timing requirement make the keep configuration impossible.
 */

/* The max num of chip select current support */
#define NUM_CHIP_SELECT		(2)
struct mrvl_nand_platform_data {
	/* the data flash bus is shared between the Static Memory
	 * Controller and the Data Flash Controller,  the arbiter
	 * controls the ownership of the bus
	 */
	int	dwidth_c;
	int	dwidth_m;

	/* allow platform code to keep OBM/bootloader defined NFC config */
	int	keep_config;

	/* indicate how many chip selects will be used */
	int	num_cs;

	/* use an flash-based bad block table */
	bool	flash_bbt;

	/* requested ECC strength and ECC step size */
	int ecc_strength, ecc_step_size;

	const struct mtd_partition		*parts[NUM_CHIP_SELECT];
	unsigned int				nr_parts[NUM_CHIP_SELECT];

	const struct mrvl_nand_flash		*flash;
	size_t					num_flash;
};

extern void mrvl_set_nand_info(struct mrvl_nand_platform_data *info);
#endif /* __MRVL_NAND_H */

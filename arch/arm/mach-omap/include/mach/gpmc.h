/**
 * @file
 * @brief This file contains the GPMC's generic definitions
 *
 * OMAP's General Purpose Memory Controller(GPMC) provides features
 * allowing us to communicate with memory devices such as NOR, NAND,
 * OneNAND, SRAM etc.. This file defines certain generic parameters
 * allowing us to configure the same painlessly.
 *
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * Originally from Linux kernel:
 * http://linux.omap.com/pub/kernel/3430zoom/linux-ldp-v1.0b.tar.gz
 * include/asm-arm/arch-omap/omap34xx.h
 *
 * Copyright (C) 2007 Texas Instruments, <www.ti.com>
 * Copyright (C) 2007 Nokia Corporation.
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

#ifndef __ASM_ARCH_OMAP_GPMC_H
#define __ASM_ARCH_OMAP_GPMC_H

extern void __iomem *omap_gpmc_base;

/** GPMC Reg Wrapper */
#define GPMC_REG(REGNAME)	(omap_gpmc_base + GPMC_##REGNAME)

#define GPMC_SYS_CONFIG		(0x10)
#define GPMC_SYS_STATUS		(0x14)
#define GPMC_IRQSTATUS		(0x18)
#define GPMC_IRQ_ENABLE		(0x1C)
#define GPMC_TIMEOUT_CONTROL	(0x40)
#define GPMC_CFG		(0x50)
#define GPMC_STATUS		(0x54)
#define GPMC_PREFETCH_CONFIG1	(0x1E0)
#define GPMC_PREFETCH_CONFIG2	(0x1E4)
#define GPMC_PREFETCH_CONTROL	(0x1EC)
#define GPMC_PREFETCH_STATUS	(0x1f0)
#define GPMC_ECC_CONFIG		(0x1F4)
#define GPMC_ECC_CONTROL	(0x1F8)
#define GPMC_ECC_SIZE_CONFIG	(0x1FC)
#define GPMC_ECC1_RESULT	(0x200)
#define GPMC_ECC2_RESULT	(0x204)
#define GPMC_ECC3_RESULT	(0x208)
#define GPMC_ECC4_RESULT	(0x20C)
#define GPMC_ECC5_RESULT	(0x210)
#define GPMC_ECC6_RESULT	(0x214)
#define GPMC_ECC7_RESULT	(0x218)
#define GPMC_ECC8_RESULT	(0x21C)
#define GPMC_ECC9_RESULT	(0x220)
#define GPMC_ECC_BCH_RESULT_0	0x240

#define GPMC_CONFIG1_0		(0x60)
#define GPMC_CONFIG1_1		(0x90)
#define GPMC_CONFIG1_2		(0xC0)
#define GPMC_CONFIG1_3		(0xF0)
#define GPMC_CONFIG1_4		(0x120)
#define GPMC_CONFIG1_5		(0x150)
#define GPMC_CONFIG1_6		(0x180)
#define GPMC_CONFIG1_7		(0x1B0)
#define GPMC_CONFIG2_0		(0x64)
#define GPMC_CONFIG2_1		(0x94)
#define GPMC_CONFIG2_2		(0xC4)
#define GPMC_CONFIG2_3		(0xF4)
#define GPMC_CONFIG2_4		(0x124)
#define GPMC_CONFIG2_5		(0x154)
#define GPMC_CONFIG2_6		(0x184)
#define GPMC_CONFIG2_7		(0x1B4)
#define GPMC_CONFIG3_0		(0x68)
#define GPMC_CONFIG3_1		(0x98)
#define GPMC_CONFIG3_2		(0xC8)
#define GPMC_CONFIG3_3		(0xF8)
#define GPMC_CONFIG3_4		(0x128)
#define GPMC_CONFIG3_5		(0x158)
#define GPMC_CONFIG3_6		(0x188)
#define GPMC_CONFIG3_7		(0x1B8)
#define GPMC_CONFIG4_0		(0x6C)
#define GPMC_CONFIG4_1		(0x9C)
#define GPMC_CONFIG4_2		(0xCC)
#define GPMC_CONFIG4_3		(0xFC)
#define GPMC_CONFIG4_4		(0x12C)
#define GPMC_CONFIG4_5		(0x15C)
#define GPMC_CONFIG4_6		(0x18C)
#define GPMC_CONFIG4_7		(0x1BC)
#define GPMC_CONFIG5_0		(0x70)
#define GPMC_CONFIG5_1		(0xA0)
#define GPMC_CONFIG5_2		(0xD0)
#define GPMC_CONFIG5_3		(0x100)
#define GPMC_CONFIG5_4		(0x130)
#define GPMC_CONFIG5_5		(0x160)
#define GPMC_CONFIG5_6		(0x190)
#define GPMC_CONFIG5_7		(0x1C0)
#define GPMC_CONFIG6_0		(0x74)
#define GPMC_CONFIG6_1		(0xA4)
#define GPMC_CONFIG6_2		(0xD4)
#define GPMC_CONFIG6_3		(0x104)
#define GPMC_CONFIG6_4		(0x134)
#define GPMC_CONFIG6_5		(0x164)
#define GPMC_CONFIG6_6		(0x194)
#define GPMC_CONFIG6_7		(0x1C4)
#define GPMC_CONFIG7_0		(0x78)
#define GPMC_CONFIG7_1		(0xA8)
#define GPMC_CONFIG7_2		(0xD8)
#define GPMC_CONFIG7_3		(0x108)
#define GPMC_CONFIG7_4		(0x138)
#define GPMC_CONFIG7_5		(0x168)
#define GPMC_CONFIG7_6		(0x198)
#define GPMC_CONFIG7_7		(0x1C8)

#define GPMC_NUM_CS		8
#define GPMC_CONFIG_CS_SIZE	(GPMC_CONFIG1_1 - GPMC_CONFIG1_0)
#define GPMC_CONFIG_REG_OFF	(GPMC_CONFIG2_0 - GPMC_CONFIG1_0)

#define GPMC_CS_NAND_COMMAND	(0x1C)
#define GPMC_CS_NAND_ADDRESS	(0x20)
#define GPMC_CS_NAND_DATA	(0x24)

#define GPMC_SIZE_128M		0x08
#define GPMC_SIZE_64M		0x0C
#define GPMC_SIZE_32M		0x0E
#define GPMC_SIZE_16M		0x0F

#define PREFETCH_FIFOTHRESHOLD_MAX	0x40
#define PREFETCH_FIFOTHRESHOLD(val)	((val) << 8)
#define GPMC_PREFETCH_STATUS_FIFO_CNT(val)	((val >> 24) & 0x7F)
#define GPMC_PREFETCH_STATUS_COUNT(val)	(val & 0x00003fff)

int gpmc_prefetch_enable(int cs, int fifo_th, int dma_mode,
				unsigned int u32_count, int is_write);
int gpmc_prefetch_reset(int cs);

#define NAND_WP_BIT		0x00000010

#ifndef __ASSEMBLY__

/** Generic GPMC configuration structure to be used to configure a
 *  chip select
 */
struct gpmc_config {
	unsigned int cfg[6];
	unsigned int base;
	unsigned char size;
};

/** Generic configuration - will reset all the cs configs. */
void gpmc_generic_init(unsigned int cfg);

/** Configuration for a specific chip select */
void gpmc_cs_config(char cs, struct gpmc_config *config);

#endif

#endif /* __ASM_ARCH_OMAP_GPMC_H */

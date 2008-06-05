/**
 * @file
 * @brief This file contains the GPMC specific register definitions
 *
 * FileName: include/asm-arm/arch-omap/gpmc.h
 *
 * Originally from Linux kernel:
 * http://linux.omap.com/pub/kernel/3430zoom/linux-ldp-v1.0b.tar.gz
 * include/asm-arm/arch-omap/omap34xx.h
 */
/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __ASM_ARCH_OMAP_GPMC_H
#define __ASM_ARCH_OMAP_GPMC_H

/** GPMC Reg Wrapper */
#define GPMC_REG(REGNAME)	(OMAP_GPMC_BASE + GPMC_##REGNAME)

#define GPMC_SYS_CONFIG		(0x10)
#define GPMC_IRQ_ENABLE		(0x1C)
#define GPMC_TIMEOUT_CONTROL	(0x40)
#define GPMC_CFG		(0x50)
#define GPMC_PREFETCH_CONFIG_1	(0x1E0)
#define GPMC_PREFETCH_CONFIG_2	(0x1E4)
#define GPMC_PREFETCH_CTRL	(0x1EC)
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

#endif /* __ASM_ARCH_OMAP_GPMC_H */

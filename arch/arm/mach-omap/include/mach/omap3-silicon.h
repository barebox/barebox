/**
 * @file
 * @brief This file contains the processor specific definitions of
 * the TI OMAP34XX. For more info on OMAP34XX,
 * See http://focus.ti.com/pdfs/wtbu/swpu114g.pdf
 *
 * FileName: include/asm-arm/arch-omap/omap3-silicon.h
 *
 * OMAP34XX base address defines go here.
 *
 * Originally from Linux kernel:
 * http://linux.omap.com/pub/kernel/3430zoom/linux-ldp-v1.0b.tar.gz
 * include/asm-arm/arch-omap/omap3-silicon.h
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

#ifndef __ASM_ARCH_OMAP3_H
#define __ASM_ARCH_OMAP3_H

/* PLEASE PLACE ONLY BASE DEFINES HERE */

/** OMAP Internal Bus Base addresses */
#define OMAP_L4_CORE_BASE	0x48000000
#define OMAP_INTC_BASE		0x48200000
#define OMAP_L4_WKUP_BASE	0x48300000
#define OMAP_L4_PER_BASE	0x49000000
#define OMAP_L4_EMU_BASE	0x54000000
#define OMAP_SGX_BASE		0x50000000
#define OMAP_IVA_BASE		0x5C000000
#define OMAP_SMX_APE_BASE	0x68000000
#define OMAP_SMS_BASE		0x6C000000
#define OMAP_SDRC_BASE		0x6D000000
#define OMAP_GPMC_BASE		0x6E000000

/** Peripheral Base Addresses */
#define OMAP_CTRL_BASE		(OMAP_L4_CORE_BASE + 0x02000)
#define OMAP_CM_BASE		(OMAP_L4_CORE_BASE + 0x04000)
#define OMAP_PRM_BASE		(OMAP_L4_WKUP_BASE + 0x06000)

#define OMAP_UART1_BASE		(OMAP_L4_CORE_BASE + 0x6A000)
#define OMAP_UART2_BASE		(OMAP_L4_CORE_BASE + 0x6C000)
#define OMAP_UART3_BASE		(OMAP_L4_PER_BASE + 0x20000)

#define OMAP_I2C1_BASE		(OMAP_L4_CORE_BASE + 0x70000)
#define OMAP_I2C2_BASE		(OMAP_L4_CORE_BASE + 0x72000)
#define OMAP_I2C3_BASE		(OMAP_L4_CORE_BASE + 0x60000)

#define OMAP_GPTIMER1_BASE	(OMAP_L4_WKUP_BASE + 0x18000)
#define OMAP_GPTIMER2_BASE	(OMAP_L4_PER_BASE + 0x32000)
#define OMAP_GPTIMER3_BASE	(OMAP_L4_PER_BASE + 0x34000)
#define OMAP_GPTIMER4_BASE	(OMAP_L4_PER_BASE + 0x36000)
#define OMAP_GPTIMER5_BASE	(OMAP_L4_PER_BASE + 0x38000)
#define OMAP_GPTIMER6_BASE	(OMAP_L4_PER_BASE + 0x3A000)
#define OMAP_GPTIMER7_BASE	(OMAP_L4_PER_BASE + 0x3C000)
#define OMAP_GPTIMER8_BASE	(OMAP_L4_PER_BASE + 0x3E000)
#define OMAP_GPTIMER9_BASE	(OMAP_L4_PER_BASE + 0x40000)
#define OMAP_GPTIMER10_BASE	(OMAP_L4_CORE_BASE + 0x86000)
#define OMAP_GPTIMER11_BASE	(OMAP_L4_CORE_BASE + 0x88000)

#define OMAP_WDTIMER2_BASE	(OMAP_L4_WKUP_BASE + 0x14000)
#define OMAP_WDTIMER3_BASE	(OMAP_L4_PER_BASE + 0x30000)

#define OMAP_32KTIMER_BASE	(OMAP_L4_WKUP_BASE + 0x20000)

#define OMAP_MMC1_BASE		(OMAP_L4_CORE_BASE + 0x9C000)
#define OMAP_MMC2_BASE		(OMAP_L4_CORE_BASE + 0xB4000)
#define OMAP_MMC3_BASE		(OMAP_L4_CORE_BASE + 0xAD000)

#define OMAP_MUSB0_BASE		(OMAP_L4_CORE_BASE + 0xAB000)

#define OMAP_GPIO1_BASE		(OMAP_L4_WKUP_BASE + 0x10000)
#define OMAP_GPIO2_BASE		(OMAP_L4_PER_BASE + 0x50000)
#define OMAP_GPIO3_BASE		(OMAP_L4_PER_BASE + 0x52000)
#define OMAP_GPIO4_BASE		(OMAP_L4_PER_BASE + 0x54000)
#define OMAP_GPIO5_BASE		(OMAP_L4_PER_BASE + 0x56000)
#define OMAP_GPIO6_BASE		(OMAP_L4_PER_BASE + 0x58000)

/** MPU WDT Definition */
#define OMAP_MPU_WDTIMER_BASE	OMAP_WDTIMER2_BASE

#define OMAP_HSUSB_OTG_BASE    (OMAP_L4_CORE_BASE + 0xAB000)
#define OMAP_USBTLL_BASE       (OMAP_L4_CORE_BASE + 0x62000)
#define OMAP_UHH_CONFIG_BASE   (OMAP_L4_CORE_BASE + 0x64000)
#define OMAP_OHCI_BASE         (OMAP_L4_CORE_BASE + 0x64400)
#define OMAP_EHCI_BASE         (OMAP_L4_CORE_BASE + 0x64800)

/** Interrupt Vector base address */
#define OMAP_SRAM_BASE		0x40200000
#define OMAP_SRAM_INTVECT	0x4020F800
#define OMAP_SRAM_INTVECT_COPYSIZE	0x64
/** Temporary stack for us to use C calls in low_level_init */
#define OMAP_SRAM_STACK		0x4020FFFC

/** Gives the silicon revision */
#define OMAP_TAP_BASE		(OMAP_L4_WKUP_BASE + 0xA000)
#define IDCODE_REG		(OMAP_TAP_BASE + 0x204)
#define DIE_ID_0		(OMAP_TAP_BASE + 0x218)
#define DIE_ID_1		(OMAP_TAP_BASE + 0x21c)
#define DIE_ID_2		(OMAP_TAP_BASE + 0x220)
#define DIE_ID_3		(OMAP_TAP_BASE + 0x224)

/** Masks to extract information from ID code register */
#define IDCODE_HAWKEYE_MASK	0x0FFFF000
#define IDCODE_VERSION_MASK	0xF0000000

 #define get_hawkeye(v)		(((v) & IDCODE_HAWKEYE_MASK) >> 12)
 #define get_version(v)		(((v) & IDCODE_VERSION_MASK) >> 28)

#define HAWKEYE_ES1		0x0B6D6000
#define HAWKEYE_ES2		0x0B7AE000
#define HAWKEYE_ES2_1		0x1B7AE000

#define DEVICE_MASK		((0x1 << 8)|(0x1 << 9)|(0x1 << 10))

#define OMAP_SDRC_CS0		0x80000000
#define OMAP_SDRC_CS1		0xA0000000

/* PRM */
#define PRM_RSTCTRL_RESET       0x04

#endif /* __ASM_ARCH_OMAP3_H */


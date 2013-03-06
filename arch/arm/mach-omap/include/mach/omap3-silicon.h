/**
 * @file
 * @brief This file contains the processor specific definitions of
 * the TI OMAP34XX. For more info on OMAP34XX,
 * See http://focus.ti.com/pdfs/wtbu/swpu114g.pdf
 *
 * OMAP34XX base address defines go here.
 *
 * Originally from Linux kernel:
 * http://linux.omap.com/pub/kernel/3430zoom/linux-ldp-v1.0b.tar.gz
 * include/asm-arm/arch-omap/omap3-silicon.h
 *
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
 */

#ifndef __ASM_ARCH_OMAP3_H
#define __ASM_ARCH_OMAP3_H

/* PLEASE PLACE ONLY BASE DEFINES HERE */

/** OMAP Internal Bus Base addresses */
#define OMAP3_L4_CORE_BASE	0x48000000
#define OMAP3_INTC_BASE		0x48200000
#define OMAP3_L4_WKUP_BASE	0x48300000
#define OMAP3_L4_PER_BASE	0x49000000
#define OMAP3_L4_EMU_BASE	0x54000000
#define OMAP3_SGX_BASE		0x50000000
#define OMAP3_IVA_BASE		0x5C000000
#define OMAP3_SMX_APE_BASE	0x68000000
#define OMAP3_SMS_BASE		0x6C000000
#define OMAP3_SDRC_BASE		0x6D000000
#define OMAP3_GPMC_BASE		0x6E000000

/** Peripheral Base Addresses */
#define OMAP3_CTRL_BASE		(OMAP3_L4_CORE_BASE + 0x02000)
#define OMAP3_CM_BASE		(OMAP3_L4_CORE_BASE + 0x04000)
#define OMAP3_PRM_BASE		(OMAP3_L4_WKUP_BASE + 0x06000)

#define OMAP3_UART1_BASE	(OMAP3_L4_CORE_BASE + 0x6A000)
#define OMAP3_UART2_BASE	(OMAP3_L4_CORE_BASE + 0x6C000)
#define OMAP3_UART3_BASE	(OMAP3_L4_PER_BASE + 0x20000)

#define OMAP3_I2C1_BASE		(OMAP3_L4_CORE_BASE + 0x70000)
#define OMAP3_I2C2_BASE		(OMAP3_L4_CORE_BASE + 0x72000)
#define OMAP3_I2C3_BASE		(OMAP3_L4_CORE_BASE + 0x60000)

#define OMAP3_GPTIMER1_BASE	(OMAP3_L4_WKUP_BASE + 0x18000)
#define OMAP3_GPTIMER2_BASE	(OMAP3_L4_PER_BASE + 0x32000)
#define OMAP3_GPTIMER3_BASE	(OMAP3_L4_PER_BASE + 0x34000)
#define OMAP3_GPTIMER4_BASE	(OMAP3_L4_PER_BASE + 0x36000)
#define OMAP3_GPTIMER5_BASE	(OMAP3_L4_PER_BASE + 0x38000)
#define OMAP3_GPTIMER6_BASE	(OMAP3_L4_PER_BASE + 0x3A000)
#define OMAP3_GPTIMER7_BASE	(OMAP3_L4_PER_BASE + 0x3C000)
#define OMAP3_GPTIMER8_BASE	(OMAP3_L4_PER_BASE + 0x3E000)
#define OMAP3_GPTIMER9_BASE	(OMAP3_L4_PER_BASE + 0x40000)
#define OMAP3_GPTIMER10_BASE	(OMAP3_L4_CORE_BASE + 0x86000)
#define OMAP3_GPTIMER11_BASE	(OMAP3_L4_CORE_BASE + 0x88000)

#define OMAP3_WDTIMER2_BASE	(OMAP3_L4_WKUP_BASE + 0x14000)
#define OMAP3_WDTIMER3_BASE	(OMAP3_L4_PER_BASE + 0x30000)

#define OMAP3_32KTIMER_BASE	(OMAP3_L4_WKUP_BASE + 0x20000)

#define OMAP3_MMC1_BASE		(OMAP3_L4_CORE_BASE + 0x9C000)
#define OMAP3_MMC2_BASE		(OMAP3_L4_CORE_BASE + 0xB4000)
#define OMAP3_MMC3_BASE		(OMAP3_L4_CORE_BASE + 0xAD000)

#define OMAP3_MUSB0_BASE	(OMAP3_L4_CORE_BASE + 0xAB000)

#define OMAP3_GPIO1_BASE	(OMAP3_L4_WKUP_BASE + 0x10000)
#define OMAP3_GPIO2_BASE	(OMAP3_L4_PER_BASE + 0x50000)
#define OMAP3_GPIO3_BASE	(OMAP3_L4_PER_BASE + 0x52000)
#define OMAP3_GPIO4_BASE	(OMAP3_L4_PER_BASE + 0x54000)
#define OMAP3_GPIO5_BASE	(OMAP3_L4_PER_BASE + 0x56000)
#define OMAP3_GPIO6_BASE	(OMAP3_L4_PER_BASE + 0x58000)

/** MPU WDT Definition */
#define OMAP3_MPU_WDTIMER_BASE	OMAP3_WDTIMER2_BASE

#define OMAP3_HSUSB_OTG_BASE    (OMAP3_L4_CORE_BASE + 0xAB000)
#define OMAP3_USBTLL_BASE       (OMAP3_L4_CORE_BASE + 0x62000)
#define OMAP3_UHH_CONFIG_BASE   (OMAP3_L4_CORE_BASE + 0x64000)
#define OMAP3_OHCI_BASE         (OMAP3_L4_CORE_BASE + 0x64400)
#define OMAP3_EHCI_BASE         (OMAP3_L4_CORE_BASE + 0x64800)

/** Interrupt Vector base address */
#define OMAP3_SRAM_BASE		0x40200000
#define OMAP3_SRAM_INTVECT	0x4020F800
#define OMAP3_SRAM_INTVECT_COPYSIZE	0x64

/** Gives the silicon revision */
#define OMAP3_TAP_BASE		(OMAP3_L4_WKUP_BASE + 0xA000)
#define OMAP3_IDCODE_REG	(OMAP3_TAP_BASE + 0x204)
#define OMAP3_DIE_ID_0		(OMAP3_TAP_BASE + 0x218)
#define OMAP3_DIE_ID_1		(OMAP3_TAP_BASE + 0x21c)
#define OMAP3_DIE_ID_2		(OMAP3_TAP_BASE + 0x220)
#define OMAP3_DIE_ID_3		(OMAP3_TAP_BASE + 0x224)

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
#define OMAP3_PRM_RSTCTRL_RESET	0x04

/*
 * ROM code API related flags
 */
#define OMAP3_GP_ROMCODE_API_L2_INVAL		1
#define OMAP3_GP_ROMCODE_API_WRITE_ACR		3

/* If Architecture specific init functions are present */
#ifndef __ASSEMBLY__
void omap3_core_init(void);
void omap3_gp_romcode_call(u32 service_id, u32 parameter);
#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARCH_OMAP3_H */

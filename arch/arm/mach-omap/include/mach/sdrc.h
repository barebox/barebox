/**
 * @file
 * @brief This file contains the SDRC specific register definitions
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 *
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#ifndef _ASM_ARCH_SDRC_H
#define _ASM_ARCH_SDRC_H

#define OMAP3_SDRC_REG(REGNAME)	(OMAP3_SDRC_BASE + OMAP_SDRC_##REGNAME)
#define OMAP_SDRC_SYSCONFIG	(0x10)
#define OMAP_SDRC_STATUS	(0x14)
#define OMAP_SDRC_CS_CFG	(0x40)
#define OMAP_SDRC_SHARING	(0x44)
#define OMAP_SDRC_DLLA_CTRL	(0x60)
#define OMAP_SDRC_DLLA_STATUS	(0x64)
#define OMAP_SDRC_DLLB_CTRL	(0x68)
#define OMAP_SDRC_DLLB_STATUS	(0x6C)
#define DLLPHASE		(0x1 << 1)
#define LOADDLL			(0x1 << 2)
#define DLL_DELAY_MASK		0xFF00
#define DLL_NO_FILTER_MASK	((0x1 << 8)|(0x1 << 9))

#define OMAP_SDRC_POWER		(0x70)
#define WAKEUPPROC		(0x1 << 26)

#define OMAP_SDRC_MCFG_0	(0x80)
#define OMAP_SDRC_MCFG_1	(0xB0)
#define OMAP_SDRC_MR_0		(0x84)
#define OMAP_SDRC_MR_1		(0xB4)
#define OMAP_SDRC_ACTIM_CTRLA_0	(0x9C)
#define OMAP_SDRC_ACTIM_CTRLB_0	(0xA0)
#define OMAP_SDRC_ACTIM_CTRLA_1	(0xC4)
#define OMAP_SDRC_ACTIM_CTRLB_1	(0xC8)
#define OMAP_SDRC_RFR_CTRL_0	(0xA4)
#define OMAP_SDRC_RFR_CTRL_1	(0xD4)
#define OMAP_SDRC_MANUAL_0	(0xA8)
#define CMD_NOP			0x0
#define CMD_PRECHARGE		0x1
#define CMD_AUTOREFRESH		0x2
#define CMD_ENTR_PWRDOWN	0x3
#define CMD_EXIT_PWRDOWN	0x4
#define CMD_ENTR_SRFRSH		0x5
#define CMD_CKE_HIGH		0x6
#define CMD_CKE_LOW		0x7
#define SOFTRESET		(0x1 << 1)
#define SMART_IDLE		(0x2 << 3)
#define REF_ON_IDLE		(0x1 << 6)

#define SDRC_CS0_OSET		0x0
/* Mirror CS1 regs appear offset 0x30 from CS0 */
#define SDRC_CS1_OSET		0x30

#define SDRC_STACKED		0
#define SDRC_IP_DDR		1
#define SDRC_COMBO_DDR		2
#define SDRC_IP_SDR		3


#define SDRC_B_R_C		(0 << 6)	/* bank-row-column */
#define SDRC_B1_R_B0_C		(1 << 6)	/* bank1-row-bank0-column */
#define SDRC_R_B_C		(2 << 6)	/* row-bank-column */

#define DLL_OFFSET              0
#define DLL_WRITEDDRCLKX2DIS    1
#define DLL_ENADLL              1
#define DLL_LOCKDLL             0
#define DLL_DLLPHASE_72         0
#define DLL_DLLPHASE_90         1

#endif /* _ASM_ARCH_SDRC_H */

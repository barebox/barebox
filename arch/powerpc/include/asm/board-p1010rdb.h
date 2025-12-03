// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009-2011 Freescale Semiconductor, Inc.

/* Don't include directly, include <asm/config.h> instead.  */

#ifndef __BOARD_P1010RDB_H
#define __BOARD_P1010RDB_H

#define CFG_SYS_CLK_FREQ	66666666
#define CFG_DDR_CLK_FREQ	66666666

#define CFG_CHIP_SELECTS_PER_CTRL 1

/*
 * Memory map
 *
 * 0x0000_0000	0x3fff_ffff	DDR		1G cacheable
 *
 * Localbus non-cacheable
 * 0xee0_0000	0xefff_ffff	FLASH		32M non-cacheable
 * 0xffb0_0000	0xffb0_0fff	PIXIS		4K Cacheable
 * 0xffd0_0000	0xffd0_3fff	L1 for stack	16K Cacheable TLB0
 */
#define CFG_SDRAM_BASE		0x00000000

#define CFG_CCSRBAR_DEFAULT	0xff700000
#define CFG_CCSRBAR		0xffe00000
#define CFG_CCSRBAR_PHYS	CFG_CCSRBAR
#define CFG_IMMR		CFG_CCSRBAR

#define CFG_INIT_RAM_ADDR	0xffd00000
#define CFG_INIT_RAM_SIZE	0x00004000
#define CFG_INIT_BI_SIZE	0x00000100
#define CFG_INIT_SP_OFFSET	(CFG_INIT_RAM_SIZE - CFG_INIT_BI_SIZE)

#define CFG_BOOT_BLOCK		0xe0000000
#define CFG_BOOT_BLOCK_PHYS	CFG_BOOT_BLOCK
#define CFG_FLASH_BASE		0xee000000
#define CFG_FLASH_BASE_PHYS	CFG_FLASH_BASE
#define CFG_CPLD_BASE		0xffb00000
#define CFG_CPLD_BASE_PHYS	CFG_CPLD_BASE

#define CFG_IFC_CSPR0	(CSPR_PHYS_ADDR(CFG_FLASH_BASE_PHYS) | \
			CSPR_PORT_SIZE_16 | CSPR_MSEL_NOR | \
			CSPR_V)
#define CFG_IFC_CSOR0	CSOR_NOR_ADM_SHIFT(7)
#define CFG_IFC_AMASK0	IFC_AMASK(32*1024*1024)

#endif /* __BOARD_P1010RDB_H */

// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009-2011 Freescale Semiconductor, Inc.

/* Don't include directly, include <asm/config.h> instead.  */

#ifndef __BOARD_P1022DS_H
#define __BOARD_P1022DS_H

#ifndef __ASSEMBLY__
extern unsigned long ics307_clk_freq(unsigned int reg);
#endif
#define CFG_SYS_CLK_FREQ	ics307_clk_freq(25)
#define CFG_DDR_CLK_FREQ	ics307_clk_freq(28)

#define CFG_CHIP_SELECTS_PER_CTRL	 2

/*
 * Memory map
 *
 * 0x0000_0000	0x7fff_ffff	DDR		2G cacheable
 *
 * Localbus non-cacheable
 * 0xe800_0000	0xefff_ffff	FLASH		128M non-cacheable
 * 0xffdf_0000	0xffdf_0fff	PIXIS		4K Cacheable
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
#define CFG_FLASH_BASE		0xe8000000
#define CFG_FLASH_BASE_PHYS	CFG_FLASH_BASE
#define CFG_PIXIS_BASE		0xffdf0000
#define CFG_PIXIS_BASE_PHYS	CFG_PIXIS_BASE

#endif /* __BOARD_P1022DS_H */

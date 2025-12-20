// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 GE Intelligent Platforms, Inc.

/* Don't include directly, include <asm/config.h> instead.  */

#ifndef __BOARD_DA923RC_H
#define __BOARD_DA923RC_H

#define CFG_SYS_CLK_FREQ	66666666

#define CFG_FLASH_CFI_WIDTH FLASH_CFI_16BIT
#define CFG_CHIP_SELECTS_PER_CTRL   1

/*
 * Memory map
 *
 * 0x0000_0000	0x1fff_ffff	DDR		512MB Cacheable
 * 0xe000_0000	0xe00f_ffff	CCSR		1M non-cacheable
 * 0xf400_0000	0xf400_3fff	L1 for stack	4K Cacheable TLB0
 *
 */
#define CFG_SDRAM_BASE		0x00000000

#define CFG_CCSRBAR_DEFAULT	0xff700000
#define CFG_CCSRBAR		0xe0000000
#define CFG_CCSRBAR_PHYS	CFG_CCSRBAR
#define CFG_IMMR		CFG_CCSRBAR

/* Initial memory for global storage and stack.  */
#define CFG_INIT_RAM_ADDR	0xf4000000
#define CFG_INIT_RAM_SIZE	0x00004000
#define CFG_INIT_BI_SIZE	0x100
#define CFG_INIT_SP_OFFSET	(CFG_INIT_RAM_SIZE - CFG_INIT_BI_SIZE)

#define BOOT_BLOCK		0xfc000000

#define BOARD_TYPE_UNKNOWN	-1
#define BOARD_TYPE_NONE		0
#define BOARD_TYPE_DA923	1
#define BOARD_TYPE_GBX460	2

#endif /* __BOARD_DA923RC_H */

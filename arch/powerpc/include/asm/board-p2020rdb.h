// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2012 GE Intelligent Platforms, Inc.
// SPDX-FileCopyrightText: 2009-2011 Freescale Semiconductor, Inc.

/* Don't include directly, include <asm/config.h> instead.  */

#ifndef __BOARD_P2020RDB_H
#define __BOARD_P2020RDB_H

#ifndef __ASSEMBLY__
extern unsigned long get_board_sys_clk(unsigned long dummy);
#endif
#define CFG_SYS_CLK_FREQ	get_board_sys_clk(0)
#define CFG_DDR_CLK_FREQ	66666666

/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT	0xff700000

#define CFG_CCSRBAR		0xffe00000	/* relocated CCSRBAR */
#define CFG_CCSRBAR_PHYS	CFG_CCSRBAR

#define CFG_IMMR		CFG_CCSRBAR

/* DDR Setup */

#define CFG_CHIP_SELECTS_PER_CTRL   1

#define CFG_SDRAM_BASE		0x00000000

/* These timings are adjusted for a 667Mhz clock. */
#define CFG_SYS_DDR_CS0_BNDS		0x0000003f	/* 1GB */
#define CFG_SYS_DDR_CS0_CONFIG	        0x80014202
#define CFG_SYS_DDR_TIMING_3		0x00030000
#define CFG_SYS_DDR_TIMING_0		0x55770802
#define CFG_SYS_DDR_TIMING_1		0x5f599543
#define CFG_SYS_DDR_TIMING_2		0x0fa074d1

#define CFG_SYS_DDR_CONTROL		0xc3000000
#define CFG_SYS_DDR_CONTROL2		0x24401000
#define CFG_SYS_DDR_MODE_1		0x00040852
#define CFG_SYS_DDR_MODE_2		0x00000000
#define CFG_SYS_MD_CNTL			0x00000000
#define CFG_SYS_DDR_INTERVAL		0x0a280100

#define CFG_SYS_DDR_DATA_INIT	        0xdeadbeef
#define CFG_SYS_DDR_CLK_CTRL		0x03000000

/*
 * Memory map
 *
 * 0x0000_0000	0x3fff_ffff	DDR			1G cacheablen
 *
 * Localbus non-cacheable
 * 0xef00_0000	0xefff_ffff	FLASH			16M non-cacheable
 * 0xffd0_0000	0xffd0_3fff	L1 for stack		16K Cacheable TLB0
 */

/*
 * Local Bus Definitions
 */
#define CFG_FLASH_BASE		0xef000000
#define CFG_FLASH_BASE_PHYS	CFG_FLASH_BASE

#define CFG_INIT_RAM_ADDR	0xffd00000	/* stack in RAM */
/* Leave 256 bytes for global data */
#define CFG_INIT_SP_OFFSET	(0x00004000 - 256)

#endif	/* __BOARD_P2020RDB_H */

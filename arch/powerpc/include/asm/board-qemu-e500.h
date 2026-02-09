/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * QEMU ppce500 virtual machine configuration
 *
 * CCSRBAR effective address is 0xE0000000, physical 0xF_E0000000
 * (36-bit, MAS7 = 0xF).
 */

#ifndef __ASM_BOARD_QEMU_E500_H
#define __ASM_BOARD_QEMU_E500_H

/*
 * Serial clock - QEMU's ns16550 baudbase is 399193, but we use
 * 1843200 (standard 16550 crystal) so the divisor calculation
 * produces a reasonable value. QEMU ignores the actual baud rate.
 */
#define CFG_SYS_CLK_FREQ	1843200

/*
 * DDR configuration - QEMU doesn't have a real DDR controller,
 * but the define is needed for compilation.
 */
#define CFG_CHIP_SELECTS_PER_CTRL	1

#define CFG_SDRAM_BASE		0x00000000

/*
 * CCSR (CCSRBAR) - SoC register space
 *
 * QEMU ppce500 maps CCSRBAR at effective address 0xE0000000,
 * physical address 0xF_E0000000 (36-bit, MAS7 = 0xF).
 */
#define CFG_CCSRBAR_DEFAULT	0xE0000000
#define CFG_CCSRBAR		0xE0000000
#define CFG_CCSRBAR_PHYS	CFG_CCSRBAR
#define CFG_CCSRBAR_PHYS_HIGH	0xF
#define CFG_IMMR		CFG_CCSRBAR

#endif /* __ASM_BOARD_QEMU_E500_H */

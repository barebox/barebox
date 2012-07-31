/**
 * @file
 * @brief Global defintions for the ARM S3C2440 based mini2440 CPU card
 */
/* This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/**
 * The external clock reference is a 12.00 MHz crystal
 */
#define S3C24XX_CLOCK_REFERENCE 12000000

/**
 * Define the main clock configuration to be used in register CLKDIVN
 *
 * We must limit the frequency of the connected SDRAMs with the clock ratio
 * setup to 1:4:8. This will result into FCLK:HCLK:PCLK = 405Mhz:102MHz:51MHz
 */
#define BOARD_SPECIFIC_CLKDIVN 0x05

/**
 * Define the MPLL configuration to be used in register MPLLCON
 *
 * We want the MPLL to run at 405.0 MHz
 */
#define BOARD_SPECIFIC_MPLL ((0x7f << 12) + (2 << 4) + 1)

/**
 * Define the UPLL configuration to be used in register UPLLCON
 *
 * We want the UPLL to run at 48.0 MHz
 */
#define BOARD_SPECIFIC_UPLL ((0x38 << 12) + (2 << 4) + 2)

/*
 * Flash access timings
 * Tacls  = 0ns (but 20ns data setup time)
 * Twrph0 = 25ns (write) 35ns (read)
 * Twrph1 = 10ns (10ns data hold time)
 * Read cycle time = 50ns
 *
 * Assumed HCLK is 100MHz
 * Tacls = 1 (-> 20ns)
 * Twrph0 = 3 (-> 40ns)
 * Twrph1 = 1 (-> 20ns)
 * Cycle time = 80ns
 */
#define MINI2440_TACLS 1
#define MINI2440_TWRPH0 3
#define MINI2440_TWRPH1 1

/* needed in the generic NAND boot code only */
#ifdef CONFIG_S3C_NAND_BOOT
# define BOARD_DEFAULT_NAND_TIMING \
	CALC_NFCONF_TIMING(MINI2440_TACLS, MINI2440_TWRPH0, MINI2440_TWRPH1)
#endif

/*
 * Needed in the generic SDRAM boot code only
 *
 * SDRAM configuration
 * Two types of SDRAM devices are common on mini2440:
 * - Two devices of HY57V561620 to form 64 MiB in bank 6 only
 *   - http://friendlyarm.net/dl.php?file=HY57V561620.pdf
 * - Two devices of MT48LC16M16 to form 64 MiB in bank 6 only
 *   - http://friendlyarm.net/dl.php?file=MT48LC16M16.pdf

 * Most of the time the CPU is specified for 400 MHz only. As the CPU frequency
 * and the SDRAM frequency are fix coupled by 4:1, the SDRAM runs at HCLCK.
 * So, we need a 100 MHz timing setup with CL=2 for the SDRAMs.
 */

/*
 * - ST7/WS7/DW7: reserved, this SDRAM bank is not used
 * - ST6/WS6/DW6: 32 bit data bus (for SDRAM usage)
 * - ST5/WS5/DW5: reserved, to be set by the board init code
 * - ST4/WS4/DW4: reserved, to be set by the board init code
 * - ST3/WS3/DW3: reserved, to be set by the board init code
 * - ST2/WS2/DW2: reserved, to be set by the board init code
 * - ST1/WS1/DW1: reserved, to be set by the board init code
 * - DW0: not to be changed
 */
#define BOARD_SPECIFIC_BWSCON ((0x3 << 28) | (0x2 << 24) | 0x333330)
/*
 *  - MT = 11 (= sync dram type)
 *  - Trcd = 00 (= CL2)
 *  - SCAN = 01 (= 9 bit collumns)
 */
#define BOARD_SPECIFIC_BANKCON6 ((0x3 << 15) + (0x0 << 2) + (0x1))
#define BOARD_SPECIFIC_BANKCON7 0 /* disabled */
/*
 * SDRAM refresh settings
 *  - REFEN = 1 (= refresh enabled)
 *  - TREFMD = 0 (= auto refresh)
 *  - Trp = 00 (= 2 RAS precharge clocks)
 *  - Tsrc = 01 (= 5 clocks -> row cycle time @100MHz 2+5=7 -> 70ns)
 *  - Refresh = 2^11 + 1 - 100 * 7.8 = 2049 - 780 = 1269
 */
#define BOARD_SPECIFIC_REFRESH ((0x1 << 23) + (0x0 << 22) + (0x0 << 20) + (0x1 << 18) + 1269)
/*
 * SDRAM banksize
 *  - BURST_EN = 1 (= burst mode enabled)
 *  - SCKE_EN = 1 (= SDRAM SCKE enabled)
 *  - SCLK_EN = 1 (= clock active only during accesses)
 *  - BK67MAP = 001 (= 64 MiB)
 */
# define BOARD_SPECIFIC_BANKSIZE ((1 << 7) + (1 << 5) + (1 << 4) + 1)
/*
 * SDRAM mode register
 * CL = 010 (= 2 clocks)
 */
# define BOARD_SPECIFIC_MRSRB6 (0x2 << 4)
# define BOARD_SPECIFIC_MRSRB7 0	/* not used */

#endif /* __CONFIG_H */

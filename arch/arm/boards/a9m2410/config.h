/**
 * @file
 * @brief Global defintions for the ARM S3C2410 based a9m2410 CPU card
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
 * The external clock reference is a 12.0MHz crystal
 */
#define S3C24XX_CLOCK_REFERENCE 12000000

/**
 * Define the main clock configuration to be used in register CLKDIVN
 *
 * We must limit the frequency of the connected SDRAMs with the clock ratio
 * setup to 1:2:4. This will result into FCLK:HCLK:PCLK = 200Mhz:100MHz:50MHz
 */
#define BOARD_SPECIFIC_CLKDIVN 0x003

/**
 * Define the MPLL configuration to be used in register MPLLCON
 *
 * We want the MPLL to run at 202.80MHz
 */
#define BOARD_SPECIFIC_MPLL ((0xA1 << 12) + (3 << 4) + 1)

/**
 * Define the UPLL configuration to be used in register UPLLCON
 *
 * We want the UPLL to run at 48.0MHz
 */
#define BOARD_SPECIFIC_UPLL ((0x78 << 12) + (2 << 4) + 3)

/*
 * SDRAM configuration for Samsung K4M563233E
 *  - 2M x 32Bit x 4 Banks Mobile SDRAM
 *  - 90 pin FBGA
 *  - CL2@100MHz
 */
/*
 * SDRAM uses 32bit width
 */
#define BOARD_SPECIFIC_BWSCON ((0x02 << 24) + (0x02 << 28))
/*
 * 32MiB SDRAM in bank6
 *  - MT = 11 (= sync dram type)
 *  - Trcd = 00 (= CL2)
 *  - SCAN = 01 (= 9 bit collumns)
 */
#define BOARD_SPECIFIC_BANKCON6 ((0x3 << 15) + (0x0 << 2) + 0x1)
/*
 * No memory in bank7
 */
#define BOARD_SPECIFIC_BANKCON7 ((0x3 << 15) + (0x0 << 2) + 0x1)
/*
 * SDRAM refresh settings
 *  - REFEN = 1 (= refresh enabled)
 *  - TREFMD = 0 (= auto refresh)
 *  - Trp = 00 (= 2 RAS precharge clocks)
 *  - Tsrc = 01 (= 5 clocks -> row cycle time @100MHz 2+5=7 -> 70ns)
 *  - Refrsh = 2^11 + 1 - 100 * 15.6 = 2049 - 1560 = 489
 */
#define BOARD_SPECIFIC_REFRESH ((0x1 << 23) + (0x0 << 22) + (0x0 << 20) + (0x1 << 18) + 489)
/*
 * SDRAM banksize
 *  - BURST_EN = 1 (= burst mode enabled)
 *  - SCKE_EN = 1 (= SDRAM SCKE enabled)
 *  - SCLK_EN = 1 (= clock active only during accesses)
 *  - BK67MAP = 000 (= 32MiB)
 */
#define BOARD_SPECIFIC_BANKSIZE ((1 << 7) + (1 << 5) + (0 << 4) + 0)
/*
 * SDRAM mode register bank6
 * CL = 010 (= 2 clocks)
 */
#define BOARD_SPECIFIC_MRSRB6 (0x2 << 4)
/*
 * SDRAM mode register bank7
 * CL = 010 (= 2 clocks)
 */
#define BOARD_SPECIFIC_MRSRB7 (0x2 << 4)

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
#define A9M2410_TACLS 1
#define A9M2410_TWRPH0 3
#define A9M2410_TWRPH1 1

/* needed in the generic NAND boot code only */
#ifdef CONFIG_S3C_NAND_BOOT
# define BOARD_DEFAULT_NAND_TIMING CALC_NFCONF_TIMING(A9M2410_TACLS, A9M2410_TWRPH0, A9M2410_TWRPH1)
#endif

#endif /* __CONFIG_H */

/**
 * @file
 * @brief Global defintions for the ARM S3C2440 based a9m2440 CPU card
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
 * The external clock reference is a 16.9344 MHz crystal
 */
#define S3C24XX_CLOCK_REFERENCE 16934400

/**
 * Define the main clock configuration to be used in register CLKDIVN
 *
 * We must limit the frequency of the connected SDRAMs with the clock ratio
 * setup to 1:4:8. This will result into FCLK:HCLK:PCLK = 400Mhz:100MHz:50MHz
 */
#define BOARD_SPECIFIC_CLKDIVN 0x05

/**
 * Define the MPLL configuration to be used in register MPLLCON
 *
 * We want the MPLL to run at 399.65 MHz
 */
#define BOARD_SPECIFIC_MPLL ((0x6e << 12) + (3 << 4) + 1)

/**
 * Define the UPLL configuration to be used in register UPLLCON
 *
 * We want the UPLL to run at 47.98 MHz
 */
#define BOARD_SPECIFIC_UPLL ((0x3c << 12) + (4 << 4) + 2)

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
#define A9M2440_TACLS 1
#define A9M2440_TWRPH0 3
#define A9M2440_TWRPH1 1

/* needed in the generic NAND boot code only */
#ifdef CONFIG_S3C_NAND_BOOT
# define BOARD_DEFAULT_NAND_TIMING CALC_NFCONF_TIMING(A9M2440_TACLS, A9M2440_TWRPH0, A9M2440_TWRPH1)
#endif

#endif /* __CONFIG_H */

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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#define A9M2440_TACLS 1
#define A9M2440_TWRPH0 3
#define A9M2440_TWRPH1 1

/* needed in the generic NAND boot code only */
#ifdef CONFIG_S3C24XX_NAND_BOOT
# define BOARD_DEFAULT_NAND_TIMING CALC_NFCONF_TIMING(A9M2440_TACLS, A9M2440_TWRPH0, A9M2440_TWRPH1)
#endif
#endif /* __CONFIG_H */

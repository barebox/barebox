/*
 * (C) Copyright 2004
 * Mark Jonas, Freescale Semiconductor, mark.jonas@motorola.com.
 *
 * Eric Schumann, Phytec Messtechnik
 * adapted for mt46v32m16-75 DDR-RAM
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
 *
 */

#define SDRAM_DDR	1		/* is DDR */

/* Settings for XLB = 132 MHz */
#define SDRAM_MODE	0x018D0000
#define SDRAM_EMODE	0x40090000
#define SDRAM_CONTROL	0x715f0f00
#define SDRAM_CONFIG1	0x73722930
#define SDRAM_CONFIG2	0x47770000


/* Settings for XLB = 99 MHz */
/*
#define SDRAM_MODE	0x008D0000
#define SDRAM_EMODE	0x40090000
#define SDRAM_CONTROL	0x714b0f00
#define SDRAM_CONFIG1	0x63611730
#define SDRAM_CONFIG2	0x47670000
*/

#define SDRAM_TAPDELAY	0x10000000 /* reserved Bit in MPC5200 B3-Step */

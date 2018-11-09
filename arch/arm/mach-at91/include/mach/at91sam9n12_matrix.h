/*
 * Matrix-Centric header file for the AT91SAM9N12 SoC
 *
 *  Copyright (C) 2011 Atmel Corporation
 *
 * Memory Controllers (MATRIX, EBI) - System peripherals registers.
 * Based on AT91SAM9N12 preliminary datasheet.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _AT91SAM9N12_MATRIX_H_
#define _AT91SAM9N12_MATRIX_H_

#define AT91SAM9N12_MATRIX_MCFG0	(0x00)	/* Master Configuration Register 0 */
#define AT91SAM9N12_MATRIX_MCFG1	(0x04)	/* Master Configuration Register 1 */
#define AT91SAM9N12_MATRIX_MCFG2	(0x08)	/* Master Configuration Register 2 */
#define AT91SAM9N12_MATRIX_MCFG3	(0x0C)	/* Master Configuration Register 3 */
#define AT91SAM9N12_MATRIX_MCFG4	(0x10)	/* Master Configuration Register 4 */
#define AT91SAM9N12_MATRIX_MCFG5	(0x14)	/* Master Configuration Register 5 */
#define		AT91SAM9N12_MATRIX_ULBT	(7 << 0)	/* Undefined Length Burst Type */
#define			AT91SAM9N12_MATRIX_ULBT_INFINITE	(0 << 0)
#define			AT91SAM9N12_MATRIX_ULBT_SINGLE		(1 << 0)
#define			AT91SAM9N12_MATRIX_ULBT_FOUR		(2 << 0)
#define			AT91SAM9N12_MATRIX_ULBT_EIGHT		(3 << 0)
#define			AT91SAM9N12_MATRIX_ULBT_SIXTEEN	(4 << 0)
#define			AT91SAM9N12_MATRIX_ULBT_THIRTYTWO	(5 << 0)
#define			AT91SAM9N12_MATRIX_ULBT_SIXTYFOUR	(6 << 0)
#define			AT91SAM9N12_MATRIX_ULBT_128		(7 << 0)

#define AT91SAM9N12_MATRIX_SCFG0	(0x40)	/* Slave Configuration Register 0 */
#define AT91SAM9N12_MATRIX_SCFG1	(0x44)	/* Slave Configuration Register 1 */
#define AT91SAM9N12_MATRIX_SCFG2	(0x48)	/* Slave Configuration Register 2 */
#define AT91SAM9N12_MATRIX_SCFG3	(0x4C)	/* Slave Configuration Register 3 */
#define AT91SAM9N12_MATRIX_SCFG4	(0x50)	/* Slave Configuration Register 4 */
#define		AT91SAM9N12_MATRIX_SLOT_CYCLE		(0x1ff << 0)	/* Maximum Number of Allowed Cycles for a Burst */
#define		AT91SAM9N12_MATRIX_DEFMSTR_TYPE	(3 << 16)	/* Default Master Type */
#define			AT91SAM9N12_MATRIX_DEFMSTR_TYPE_NONE	(0 << 16)
#define			AT91SAM9N12_MATRIX_DEFMSTR_TYPE_LAST	(1 << 16)
#define			AT91SAM9N12_MATRIX_DEFMSTR_TYPE_FIXED	(2 << 16)
#define		AT91SAM9N12_MATRIX_FIXED_DEFMSTR	(0xf << 18)	/* Fixed Index of Default Master */

#define AT91SAM9N12_MATRIX_PRAS0	(0x80)	/* Priority Register A for Slave 0 */
#define AT91SAM9N12_MATRIX_PRAS1	(0x88)	/* Priority Register A for Slave 1 */
#define AT91SAM9N12_MATRIX_PRAS2	(0x90)	/* Priority Register A for Slave 2 */
#define AT91SAM9N12_MATRIX_PRAS3	(0x98)	/* Priority Register A for Slave 3 */
#define AT91SAM9N12_MATRIX_PRAS4	(0xA0)	/* Priority Register A for Slave 4 */
#define		AT91SAM9N12_MATRIX_M0PR		(3 << 0)	/* Master 0 Priority */
#define		AT91SAM9N12_MATRIX_M1PR		(3 << 4)	/* Master 1 Priority */
#define		AT91SAM9N12_MATRIX_M2PR		(3 << 8)	/* Master 2 Priority */
#define		AT91SAM9N12_MATRIX_M3PR		(3 << 12)	/* Master 3 Priority */
#define		AT91SAM9N12_MATRIX_M4PR		(3 << 16)	/* Master 4 Priority */
#define		AT91SAM9N12_MATRIX_M5PR		(3 << 20)	/* Master 5 Priority */

#define AT91SAM9N12_MATRIX_MRCR	(0x100)	/* Master Remap Control Register */
#define		AT91SAM9N12_MATRIX_RCB0		(1 << 0)	/* Remap Command for AHB Master 0 (ARM926EJ-S Instruction Master) */
#define		AT91SAM9N12_MATRIX_RCB1		(1 << 1)	/* Remap Command for AHB Master 1 (ARM926EJ-S Data Master) */
#define		AT91SAM9N12_MATRIX_RCB2		(1 << 2)
#define		AT91SAM9N12_MATRIX_RCB3		(1 << 3)
#define		AT91SAM9N12_MATRIX_RCB4		(1 << 4)
#define		AT91SAM9N12_MATRIX_RCB5		(1 << 5)

#define AT91SAM9N12_MATRIX_EBICSA	(0x118)	/* EBI Chip Select Assignment Register */
#define		AT91SAM9N12_MATRIX_EBI_CS1A		(1 << 1)	/* Chip Select 1 Assignment */
#define			AT91SAM9N12_MATRIX_EBI_CS1A_SMC		(0 << 1)
#define			AT91SAM9N12_MATRIX_EBI_CS1A_SDRAMC		(1 << 1)
#define		AT91SAM9N12_MATRIX_EBI_CS3A		(1 << 3)	/* Chip Select 3 Assignment */
#define			AT91SAM9N12_MATRIX_EBI_CS3A_SMC		(0 << 3)
#define			AT91SAM9N12_MATRIX_EBI_CS3A_SMC_NANDFLASH	(1 << 3)
#define		AT91SAM9N12_MATRIX_EBI_DBPUC		(1 << 8)	/* Data Bus Pull-up Configuration */
#define			AT91SAM9N12_MATRIX_EBI_DBPU_ON			(0 << 8)
#define			AT91SAM9N12_MATRIX_EBI_DBPU_OFF		(1 << 8)
#define		AT91SAM9N12_MATRIX_EBI_DBPDC		(1 << 9)	/* Data Bus Pull-up Configuration */
#define			AT91SAM9N12_MATRIX_EBI_DBPD_ON			(0 << 9)
#define			AT91SAM9N12_MATRIX_EBI_DBPD_OFF		(1 << 9)
#define		AT91SAM9N12_MATRIX_EBI_DRIVE		(1 << 17)	/* EBI I/O Drive Configuration */
#define			AT91SAM9N12_MATRIX_EBI_LOW_DRIVE		(0 << 17)
#define			AT91SAM9N12_MATRIX_EBI_HIGH_DRIVE		(1 << 17)
#define		AT91SAM9N12_MATRIX_NFD0_SELECT		(1 << 24)	/* NAND Flash Data Bus Selection */
#define			AT91SAM9N12_MATRIX_NFD0_ON_D0			(0 << 24)
#define			AT91SAM9N12_MATRIX_NFD0_ON_D16			(1 << 24)

#define AT91SAM9N12_MATRIX_WPMR	(0x1E4)	/* Write Protect Mode Register */
#define		AT91SAM9N12_MATRIX_WPMR_WPEN		(1 << 0)	/* Write Protect ENable */
#define			AT91SAM9N12_MATRIX_WPMR_WP_WPDIS		(0 << 0)
#define			AT91SAM9N12_MATRIX_WPMR_WP_WPEN		(1 << 0)
#define		AT91SAM9N12_MATRIX_WPMR_WPKEY		(0xFFFFFF << 8)	/* Write Protect KEY */

#define AT91SAM9N12_MATRIX_WPSR	(0x1E8)	/* Write Protect Status Register */
#define		AT91SAM9N12_MATRIX_WPSR_WPVS		(1 << 0)	/* Write Protect Violation Status */
#define			AT91SAM9N12_MATRIX_WPSR_NO_WPV		(0 << 0)
#define			AT91SAM9N12_MATRIX_WPSR_WPV		(1 << 0)
#define		AT91SAM9N12_MATRIX_WPSR_WPVSRC		(0xFFFF << 8)	/* Write Protect Violation Source */

#endif

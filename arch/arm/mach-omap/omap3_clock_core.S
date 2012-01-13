/**
 * @file
 * @brief Provides PRCM divisors and SRAM execution code.
 *
 * FileName: arch/arm/mach-omap/omap3_clock_core.S
 *
 * This provides two things:
 * @li @ref omap3_clock.c cannot have switch or global variables.
 * This file provides the constant data for the file to use.
 *
 * @li @ref prcm_init cannot execute certain critical clock
 * configurations while running in SDRAM/Flash. This provides
 * relocation and execution capability for the same.
 *
 * Orignally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 */
/*
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <mach/silicon.h>
#include <mach/clocks.h>
#include <mach/gpmc.h>

	/* the literal pools origin */
	.ltorg

/* MPU DPLL Parameter table
 *
 * This table defines the DPLL parameter table for the MPU as defined by
 * "struct dpll_param defined" in "omap3-clock.h"
 *
 * The tables are defined for separately each silicon revision.
 */
.globl mpu_dpll_param_34x_es1
mpu_dpll_param_34x_es1:
/*    	M	N	FREQSEL	M2	*/
.word	0x0FE,	0x07,	0x05,	0x01		/* 12   MHz	*/
.word	0x17D,	0x0C,	0x03,	0x01		/* 13   MHz	*/
.word	0x179,	0x12,	0x04,	0x01		/* 19.2 MHz	*/
.word	0x17D,	0x19,	0x03,	0x01		/* 26   MHz	*/
.word	0x1FA,	0x32,	0x03,	0x01		/* 38.4 MHz	*/

.globl mpu_dpll_param_34x_es2
mpu_dpll_param_34x_es2:
/*    	M	N	FREQSEL	M2	*/
.word	0x0FA,	0x05,	0x07,	0x01		/* 12   MHz	*/
.word	0x1F4,	0x0C,	0x03,	0x01		/* 13   MHz	*/
.word	0x271,	0x17,	0x03,	0x01		/* 19.2 MHz	*/
.word	0x0FA,	0x0C,	0x07,	0x01		/* 26   MHz	*/
.word	0x271,	0x2F,	0x03,	0x01		/* 38.4 MHz	*/

/**
 * @brief Get address of MPU DPLL param table (OMAP34XX).
 *
 * @param rev Silicon revision.
 *
 * @return Address of the param table
 */
.globl get_mpu_dpll_param_34x
get_mpu_dpll_param_34x:
	mov	r3, r0
	lsl	r3, r3, #16		/* Isolate silicon revision */
	lsr	r3, r3, #16
	cmp	r3, #0			/* Revision 1 ? */
	adr	r0, mpu_dpll_param_34x_es1
	bxeq	lr
	adr	r0, mpu_dpll_param_34x_es2
	mov	pc, lr

iva_dpll_param_34x_es1:
/*    	M	N	FREQSEL	M2				*/
.word	0x07D,	0x05,	0x07,	0x01		/* 12   MHz	*/
.word	0x0FA,	0x0C,	0x03,	0x01		/* 13   MHz	*/
.word	0x082,	0x09,	0x07,	0x01		/* 19.2 MHz	*/
.word	0x07D,	0x0C,	0x07,	0x01		/* 26   MHz	*/
.word	0x13F,	0x30,	0x03,	0x01		/* 38.4 MHz	*/

iva_dpll_param_34x_es2:
/*    	M	N	FREQSEL	M2				*/
.word	0x0B4,	0x05,	0x07,	0x01		/* 12   MHz	*/
.word	0x168,	0x0C,	0x03,	0x01		/* 13   MHz	*/
.word	0x0E1,	0x0B,	0x06,	0x01		/* 19.2 MHz	*/
.word	0x0B4,	0x0C,	0x07,	0x01		/* 26   MHz	*/
.word	0x0E1,	0x17,	0x06,	0x01		/* 38.4 MHz	*/

/**
 * @brief Get address of IVA DPLL param table (OMAP34XX).
 *
 * @param rev Silicon revision.
 *
 * @return Address of the param table
 */
.globl get_iva_dpll_param_34x
get_iva_dpll_param_34x:
	mov	r3, r0
	lsl	r3, r3, #16		/* Isolate silicon revision */
	lsr	r3, r3, #16
	cmp	r3, #0			/* Revision 1 ? */
	adr	r0, iva_dpll_param_34x_es1
	bxeq	lr
	adr	r0, iva_dpll_param_34x_es2
	mov	pc, lr

core_dpll_param_34x_es1:
/*    	M	N	FREQSEL	M2				*/
.word	0x19F,	0x0E,	0x03,	0x01		/* 12   MHz	*/
.word	0x1B2,	0x10,	0x03,	0x01		/* 13   MHz	*/
.word	0x19F,	0x17,	0x03,	0x01		/* 19.2 MHz	*/
.word	0x1B2,	0x21,	0x03,	0x01		/* 26   MHz	*/
.word	0x19F,	0x2F,	0x03,	0x01		/* 38.4 MHz	*/

core_dpll_param_34x_es2:
/*    	M	N	FREQSEL	M2				*/
.word	0x0A6,	0x05,	0x07,	0x01		/* 12   MHz	*/
.word	0x14C,	0x0C,	0x03,	0x01		/* 13   MHz	*/
.word	0x19F,	0x17,	0x03,	0x01		/* 19.2 MHz	*/
.word	0x0A6,	0x0C,	0x07,	0x01		/* 26   MHz	*/
.word	0x19F,	0x2F,	0x03,	0x01		/* 38.4 MHz	*/

/**
 * @brief Get address of CORE DPLL param table (OMAP34XX).
 *
 * @param rev Silicon revision.
 *
 * @return Address of the param table
 */
.globl get_core_dpll_param_34x
get_core_dpll_param_34x:
	mov	r3, r0
	lsl	r3, r3, #16		/* Isolate silicon revision */
	lsr	r3, r3, #16
	cmp	r3, #0			/* Revision 1 ? */
	adr	r0, core_dpll_param_34x_es1
	bxeq	lr
	adr	r0, core_dpll_param_34x_es2
	mov	pc, lr

/* PER DPLL values are same for both ES1 and ES2 */
per_dpll_param_34x:
/*    	M	N	FREQSEL	M2				*/
.word	0x0D8,	0x05,	0x07,	0x09		/* 12   MHz	*/
.word	0x1B0,	0x0C,	0x03,	0x09		/* 13   MHz	*/
.word	0x0E1,	0x09,	0x07,	0x09		/* 19.2 MHz	*/
.word	0x0D8,	0x0C,	0x07,	0x09		/* 26   MHz	*/
.word	0x0E1,	0x13,	0x07,	0x09		/* 38.4 MHz	*/

/**
 * @brief Get address of PER DPLL param table (OMAP34XX).
 *
 * @param rev Silicon revision (not used).
 *
 * @return Address of the param table
 */
.globl get_per_dpll_param_34x
get_per_dpll_param_34x:
	adr r0, per_dpll_param_34x
	mov pc, lr

.globl mpu_dpll_param_36x
mpu_dpll_param_36x:
/* FIXME: All values correspond to 26MHz only */
/*    	M	N	FREQSEL	M2	*/
.word	0x12C,	0x0C,	0x00,	0x01		/* 12   MHz	*/
.word	0x12C,	0x0C,	0x00,	0x01		/* 13   MHz	*/
.word	0x12C,	0x0C,	0x00,	0x01		/* 19.2 MHz	*/
.word	0x12C,	0x0C,	0x00,	0x01		/* 26   MHz	*/
.word	0x12C,	0x0C,	0x00,	0x01		/* 38.4 MHz	*/

/**
 * @brief Get address of MPU DPLL param table (OMAP36XX).
 *
 * @param rev Silicon revision.
 *
 * @return Address of the param table
 */
.globl get_mpu_dpll_param_36x
get_mpu_dpll_param_36x:
	adr r0, mpu_dpll_param_36x
	mov pc, lr

.globl iva_dpll_param_36x
iva_dpll_param_36x:
/* FIXME: All values correspond to 26MHz only */
/*    	M	N	FREQSEL	M2	*/
.word	0x00A,	0x00,	0x00,	0x01		/* 12   MHz	*/
.word	0x00A,	0x00,	0x00,	0x01		/* 13   MHz	*/
.word	0x00A,	0x00,	0x00,	0x01		/* 19.2 MHz	*/
.word	0x00A,	0x00,	0x00,	0x01		/* 26   MHz	*/
.word	0x00A,	0x00,	0x00,	0x01		/* 38.4 MHz	*/

/**
 * @brief Get address of IVA DPLL param table (OMAP36XX).
 *
 * @param rev Silicon revision.
 *
 * @return Address of the param table
 */
.globl get_iva_dpll_param_36x
get_iva_dpll_param_36x:
	adr r0, iva_dpll_param_36x
	mov pc, lr

.globl core_dpll_param_36x
core_dpll_param_36x:
/* FIXME: All values correspond to 26MHz only */
/*    	M	N	FREQSEL	M2	*/
.word	0x0C8,	0x0C,	0x00,	0x01		/* 12   MHz	*/
.word	0x0C8,	0x0C,	0x00,	0x01		/* 13   MHz	*/
.word	0x0C8,	0x0C,	0x00,	0x01		/* 19.2 MHz	*/
.word	0x0C8,	0x0C,	0x00,	0x01		/* 26   MHz	*/
.word	0x0C8,	0x0C,	0x00,	0x01		/* 38.4 MHz	*/

/**
 * @brief Get address of IVA DPLL param table (OMAP36XX).
 *
 * @param rev Silicon revision.
 *
 * @return Address of the param table
 */
.globl get_core_dpll_param_36x
get_core_dpll_param_36x:
	adr r0, core_dpll_param_36x
	mov pc, lr

.globl per_dpll_param_36x
per_dpll_param_36x:
/* FIXME: All values correspond to 26MHz only */
/*	M	N	M2	M3	M4	M5	M6	m2DIV */
.word	0x1B0,	0x0C,	9,	0x10,	9,	4,	3,	1 /* 12   MHz */
.word	0x1B0,	0x0C,	9,	0x10,	9,	4,	3,	1 /* 13   MHz */
.word	0x1B0,	0x0C,	9,	0x10,	9,	4,	3,	1 /* 19.2 MHz */
.word	0x1B0,	0x0C,	9,	0x10,	9,	4,	3,	1 /* 26   MHz */
.word	0x1B0,	0x0C,	9,	0x10,	9,	4,	3,	1 /* 38.4 MHz */

/**
 * @brief Get address of PER DPLL param table (OMAP36XX).
 *
 * @param rev Silicon revision.
 *
 * @return Address of the param table
 */
.globl get_per_dpll_param_36x
get_per_dpll_param_36x:
	adr r0, per_dpll_param_36x
	mov pc, lr

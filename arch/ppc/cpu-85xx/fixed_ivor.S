/*
 * Copyright 2009 Freescale Semiconductor, Inc.
 *
 * Kumar Gala <kumar.gala@freescale.com>
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

/* This file is intended to be included by other asm code since
 * we will want to execute this on both the primary core when
 * it does a bootm and the secondary core's that get released
 * out of the spin table.
 */

#define SET_IVOR(vector_number, vector_offset)	\
	li	r3,vector_offset@l; 		\
	mtspr	SPRN_IVOR##vector_number,r3;

#define SET_GIVOR(vector_number, vector_offset)	\
	li	r3,vector_offset@l; 		\
	mtspr	SPRN_GIVOR##vector_number,r3;

	SET_IVOR(0, 0x020) /* Critical Input */
	SET_IVOR(1, 0x000) /* Machine Check */
	SET_IVOR(2, 0x060) /* Data Storage */
	SET_IVOR(3, 0x080) /* Instruction Storage */
	SET_IVOR(4, 0x0a0) /* External Input */
	SET_IVOR(5, 0x0c0) /* Alignment */
	SET_IVOR(6, 0x0e0) /* Program */
	SET_IVOR(7, 0x100) /* FP Unavailable */
	SET_IVOR(8, 0x120) /* System Call */
	SET_IVOR(9, 0x140) /* Auxiliary Processor Unavailable */
	SET_IVOR(10, 0x160) /* Decrementer */
	SET_IVOR(11, 0x180) /* Fixed Interval Timer */
	SET_IVOR(12, 0x1a0) /* Watchdog Timer */
	SET_IVOR(13, 0x1c0) /* Data TLB Error */
	SET_IVOR(14, 0x1e0) /* Instruction TLB Error */
	SET_IVOR(15, 0x040) /* Debug */

	/* e500v1 & e500v2 only */
	SET_IVOR(32, 0x200) /* SPE Unavailable */
	SET_IVOR(33, 0x220) /* Embedded FP Data */
	SET_IVOR(34, 0x240) /* Embedded FP Round */

	SET_IVOR(35, 0x260) /* Performance monitor */

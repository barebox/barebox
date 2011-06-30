/*
 * Copyright (C) 2007 Guennadi Liakhovetski <lg@denx.de>
 * Copyright (C) 2009 Freescale Semiconductor, Inc.
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

#include <config.h>
#include <mach/imx-regs.h>
#include <mach/clock-imx51_53.h>

/*
 * L2CC Cache setup/invalidation/disable
 */
.macro init_l2cc
	/* explicitly disable L2 cache */
	mrc 15, 0, r0, c1, c0, 1
	bic r0, r0, #0x2
	mcr 15, 0, r0, c1, c0, 1

	/* reconfigure L2 cache aux control reg */
	mov r0, #0xC0			/* tag RAM */
	add r0, r0, #0x4		/* data RAM */
	orr r0, r0, #(1 << 24)		/* disable write allocate delay */
	orr r0, r0, #(1 << 23)		/* disable write allocate combine */
	orr r0, r0, #(1 << 22)		/* disable write allocate */

	cmp r3, #0x10			/* r3 contains the silicon rev */

	/* disable write combine for TO 2 and lower revs */
	orrls r0, r0, #(1 << 25)

	mcr 15, 1, r0, c9, c0, 2
.endm /* init_l2cc */

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr r0, =MX53_AIPS1_BASE_ADDR
	ldr r1, =0x77777777
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]

	ldr r0, =MX53_AIPS2_BASE_ADDR
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]
	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
.endm /* init_aips */

.macro setup_pll pll, freq
	ldr r0, =\pll
	ldr r1, =0x00001232
	str r1, [r0, #MX5_PLL_DP_CTL]		/* Set DPLL ON (set UPEN bit): BRMO=1 */
	mov r1, #0x2
	str r1, [r0, #MX5_PLL_DP_CONFIG]	/* Enable auto-restart AREN bit */

	ldr r1, W_DP_OP_\freq
	str r1, [r0, #MX5_PLL_DP_OP]
	str r1, [r0, #MX5_PLL_DP_HFS_OP]

	ldr r1, W_DP_MFD_\freq
	str r1, [r0, #MX5_PLL_DP_MFD]
	str r1, [r0, #MX5_PLL_DP_HFS_MFD]

	ldr r1, W_DP_MFN_\freq
	str r1, [r0, #MX5_PLL_DP_MFN]
	str r1, [r0, #MX5_PLL_DP_HFS_MFN]

	ldr r1, =0x00001232
	str r1, [r0, #MX5_PLL_DP_CTL]
1:	ldr r1, [r0, #MX5_PLL_DP_CTL]
	ands r1, r1, #0x1
	beq 1b
.endm

.macro init_clock
	ldr r0, =MX53_CCM_BASE_ADDR


	/* Switch ARM to step clock */
	mov r1, #0x4
	str r1, [r0, #MX5_CCM_CCSR]

	setup_pll MX53_PLL1_BASE_ADDR, 1000
	setup_pll MX53_PLL3_BASE_ADDR, 216

	/* Set the platform clock dividers */
	ldr r0, =MX53_ARM_BASE_ADDR
	ldr r1, =0x00000725
	str r1, [r0, #0x14]

	ldr r0, =MX53_CCM_BASE_ADDR
	mov r1, #0
	str r1, [r0, #MX5_CCM_CACRR]

	/* Switch ARM back to PLL 1 */
	mov r1, #0
	str r1, [r0, #MX5_CCM_CCSR]


	/* Restore the default values in the Gate registers */
	ldr r1, =0xFFFFFFFF
	str r1, [r0, #MX5_CCM_CCGR0]
	str r1, [r0, #MX5_CCM_CCGR1]
	str r1, [r0, #MX5_CCM_CCGR2]
	str r1, [r0, #MX5_CCM_CCGR3]
	str r1, [r0, #MX5_CCM_CCGR4]
	str r1, [r0, #MX5_CCM_CCGR5]
	str r1, [r0, #MX5_CCM_CCGR6]
#if 0
	str r1, [r0, #MX5_CCM_CCGR7]
#endif

	ldr r1, [r0, #MX5_CCM_CSCDR1]
	orr r1, r1, #0x3f
	eor r1, r1, #0x3f
	orr r1, r1, #0x21
	str r1, [r0, #MX5_CCM_CSCDR1]
	/* make sure divider effective */
1:	ldr r1, [r0, #MX5_CCM_CDHIPR]
	cmp r1, #0x0
	bne 1b

	mov r1, #0x0
	str r1, [r0, #MX5_CCM_CCDR]

	/* for cko - for ARM div by 8 */
	mov r1, #0x000A0000
	add r1, r1, #0x00000F0
	str r1, [r0, #MX5_CCM_CCOSR]
.endm

.globl board_init_lowlevel
board_init_lowlevel:
	mov r10, lr

	init_l2cc
	init_aips
	init_clock

	mov pc, r10

/* Board level setting value */
W_DP_OP_1000:		.word MX5_PLL_DP_OP_1000
W_DP_MFD_1000:		.word MX5_PLL_DP_MFD_1000
W_DP_MFN_1000:		.word MX5_PLL_DP_MFN_1000
W_DP_OP_800:		.word MX5_PLL_DP_OP_800
W_DP_MFD_800:		.word MX5_PLL_DP_MFD_800
W_DP_MFN_800:		.word MX5_PLL_DP_MFN_800
W_DP_OP_665:		.word MX5_PLL_DP_OP_665
W_DP_MFD_665:		.word MX5_PLL_DP_MFD_665
W_DP_MFN_665:		.word MX5_PLL_DP_MFN_665
W_DP_OP_216:		.word MX5_PLL_DP_OP_216
W_DP_MFD_216:		.word MX5_PLL_DP_MFD_216
W_DP_MFN_216:		.word MX5_PLL_DP_MFN_216

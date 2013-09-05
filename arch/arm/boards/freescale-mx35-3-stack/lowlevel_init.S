/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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

#include <mach/imx35-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <asm/cache-l2x0.h>
#include <asm-generic/memory_layout.h>
#include <asm/barebox-arm-head.h>

#include "board-mx35_3stack.h"

#define CSD0_BASE_ADDR		0x80000000
#define CSD1_BASE_ADDR		0x90000000
#define ESDCTL_BASE_ADDR	0xB8001000

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,	[r0];

#define writeb(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	strb		r1,	[r0];

/* Assuming 24MHz input clock */
#define MPCTL_PARAM_399	(IMX_PLL_PD(0) | IMX_PLL_MFD(15) | IMX_PLL_MFI(8) | IMX_PLL_MFN(5))
#define MPCTL_PARAM_532	((1 << 31) | IMX_PLL_PD(0) | IMX_PLL_MFD(11) | IMX_PLL_MFI(11) | IMX_PLL_MFN(1))
#define PPCTL_PARAM_300	(IMX_PLL_PD(0) | IMX_PLL_MFD(3) | IMX_PLL_MFI(6) | IMX_PLL_MFN(1))

	.section ".text_bare_init","ax"

ARM_PPMRR:		.word	0x40000015
L2CACHE_PARAM:		.word	0x00030024
CCM_CCMR_W:		.word	0x003F4208
CCM_PDR0_W:		.word	0x00001000
MPCTL_PARAM_399_W:	.word	MPCTL_PARAM_399
MPCTL_PARAM_532_W:	.word	MPCTL_PARAM_532
PPCTL_PARAM_W:		.word	PPCTL_PARAM_300
CCM_BASE_ADDR_W:	.word	MX35_CCM_BASE_ADDR

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:
	bl	arm_cpu_lowlevel_init

	mrc	15, 0, r1, c1, c0, 0

	mrc	15, 0, r0, c1, c0, 1
	orr	r0, r0, #7
	mcr	15, 0, r0, c1, c0, 1

	orr	r1, r1, #(1 << 11)		/* Flow prediction (Z) */
	orr	r1, r1, #(1 << 22)		/* unaligned accesses */
	orr	r1, r1, #(1 << 21)		/* Low Int Latency */

	mcr	15, 0, r1, c1, c0, 0

	mov	r0, #0
	mcr	15, 0, r0, c15, c2, 4

	/*
	 * Branch predicition is now enabled. Flush the BTAC to ensure a valid
	 * starting point. Don't flush BTAC while it is disabled to avoid
	 * ARM1136 erratum 408023.
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 6		/* flush entire BTAC */

	mov	r0, #0
	mcr	15, 0, r0, c7, c7, 0		/* invalidate I cache and D cache */
	mcr	15, 0, r0, c8, c7, 0		/* invalidate TLBs */
	mcr	15, 0, r0, c7, c10, 4		/* Drain the write buffer */

	/* Also setup the Peripheral Port Remap register inside the core */
	ldr	r0, ARM_PPMRR			/* start from AIPS 2GB region */
	mcr	p15, 0, r0, c15, c2, 4

/*
 * End of ARM1136 init
 */
	ldr	r0, CCM_BASE_ADDR_W

	ldr	r2, CCM_CCMR_W
	str	r2, [r0, #MX35_CCM_CCMR]

	ldr	r3, MPCTL_PARAM_532_W		/* consumer path*/

	/* Set MPLL, arm clock and ahb clock */
	str	r3, [r0, #MX35_CCM_MPCTL]

	ldr	r1, PPCTL_PARAM_W
	str	r1, [r0, #MX35_CCM_PPCTL]

	ldr	r1, CCM_PDR0_W
	str	r1, [r0, #MX35_CCM_PDR0]

	ldr	r1, [r0, #MX35_CCM_CGR0]
	orr	r1, r1, #0x00300000
	str	r1, [r0, #MX35_CCM_CGR0]

	ldr	r1, [r0, #MX35_CCM_CGR1]
	orr	r1, r1, #0x00000C00
	orr	r1, r1, #0x00000003
	str	r1, [r0, #MX35_CCM_CGR1]

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #CSD0_BASE_ADDR
	bls	1f
	cmp	pc, #CSD1_BASE_ADDR
	bhi	1f

	b	imx35_barebox_entry

1:
	ldr	r0, =ESDCTL_BASE_ADDR
	mov	r3, #0x2000
	str	r3, [r0, #0x0]
	str	r3, [r0, #0x8]

	/* ip(r12) has used to save lr register in upper calling */
	mov	fp, lr

	/* setup bank 0 */
	mov	r5, #0x00
	mov	r2, #0x00
	mov	r1, #MX35_CSD0_BASE_ADDR
	bl	setup_sdram_bank

	/* setup bank 1 */
	mov	r5, #0x00
	mov	r2, #0x00
	mov	r1, #MX35_CSD1_BASE_ADDR
	bl	setup_sdram_bank

	mov	lr, fp

	ldr	r3, =ESDCTL_DELAY_LINE5
	str	r3, [r0, #0x30]

#ifdef CONFIG_NAND_IMX_BOOT
	/* Setup a temporary stack in internal SRAM */
	ldr	sp, =MX35_IRAM_BASE_ADDR + MX35_IRAM_SIZE - 4

	b	imx35_barebox_boot_nand_external
#endif /* CONFIG_NAND_IMX_BOOT */

	b	imx35_barebox_entry

/*
 * r0: ESDCTL control base, r1: sdram slot base
 * r2: DDR type (0: DDR2, 1: MDDR) r3, r4: working base
 */
setup_sdram_bank:
	mov	r3, #0xE			/* 0xA + 0x4 */
	tst	r2, #0x1
	orreq	r3, r3, #0x300			/* DDR2 */
	str	r3, [r0, #0x10]
	bic	r3, r3, #0x00A
	str	r3, [r0, #0x10]
	beq	2f

	mov	r3, #0x20000
1:	subs	r3, r3, #1
	bne	1b

2:	tst	r2, #0x1
	ldreq	r3, =ESDCTL_DDR2_CONFIG
	ldrne	r3, =ESDCTL_MDDR_CONFIG
	cmp	r1, #CSD1_BASE_ADDR
	strlo	r3, [r0, #0x4]
	strhs	r3, [r0, #0xC]

	ldr	r3, =ESDCTL_0x92220000
	strlo	r3, [r0, #0x0]
	strhs	r3, [r0, #0x8]
	mov	r3, #0xDA
	ldr	r4, =ESDCTL_PRECHARGE
	strb	r3, [r1, r4]

	tst	r2, #0x1
	bne	skip_set_mode

	cmp	r1, #CSD1_BASE_ADDR
	ldr	r3, =ESDCTL_0xB2220000
	strlo	r3, [r0, #0x0]
	strhs	r3, [r0, #0x8]
	mov	r3, #0xDA
	ldr	r4, =ESDCTL_DDR2_EMR2
	strb	r3, [r1, r4]
	ldr	r4, =ESDCTL_DDR2_EMR3
	strb	r3, [r1, r4]
	ldr	r4, =ESDCTL_DDR2_EN_DLL
	strb	r3, [r1, r4]
	ldr	r4, =ESDCTL_DDR2_RESET_DLL
	strb	r3, [r1, r4]

	ldr	r3, =ESDCTL_0x92220000
	strlo	r3, [r0, #0x0]
	strhs	r3, [r0, #0x8]
	mov	r3, #0xDA
	ldr	r4, =ESDCTL_PRECHARGE
	strb	r3, [r1, r4]

skip_set_mode:
	cmp	r1, #CSD1_BASE_ADDR
	ldr	r3, =ESDCTL_0xA2220000
	strlo	r3, [r0, #0x0]
	strhs	r3, [r0, #0x8]
	mov	r3, #0xDA
	strb	r3, [r1]
	strb	r3, [r1]

	ldr	r3, =ESDCTL_0xB2220000
	strlo	r3, [r0, #0x0]
	strhs	r3, [r0, #0x8]
	tst	r2, #0x1
	ldreq	r4, =ESDCTL_DDR2_MR
	ldrne	r4, =ESDCTL_MDDR_MR
	mov	r3, #0xDA
	strb	r3, [r1, r4]
	ldreq	r4, =ESDCTL_DDR2_OCD_DEFAULT
	streqb	r3, [r1, r4]
	ldreq	r4, =ESDCTL_DDR2_EN_DLL
	ldrne	r4, =ESDCTL_MDDR_EMR
	strb	r3, [r1, r4]

	cmp	r1, #CSD1_BASE_ADDR
	ldr	r3, =ESDCTL_0x82228080
	strlo	r3, [r0, #0x0]
	strhs	r3, [r0, #0x8]

	tst	r2, #0x1
	moveq	r4, #0x20000
	movne	r4, #0x200
1:	subs	r4, r4, #1
	bne	1b

	str	r3, [r1, #0x100]
	ldr	r4, [r1, #0x100]
	cmp	r3, r4
	movne	r3, #1
	moveq	r3, #0

	mov	pc, lr

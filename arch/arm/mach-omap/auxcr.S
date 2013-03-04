/*
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/linkage.h>
#include <asm/unified.h>

.arm
ENTRY(setup_auxcr)
	mov	r12, #0x3
	mrc	p15, 0, r0, c1, c0, 1
	orr	r0, r0, #0x10		@ Enable ASA
	orr	r0, r0, #1 << 5		@ Enable L1NEON
	.word 0xE1600070		@ SMC
	mov	r12, #0x2
	mrc	p15, 1, r0, c9, c0, 2
	@ Set PLD_FWD bit in L2AUXCR (Cortex-A8 erratum 725233 workaround)
	orr	r0, r0, #1 << 27
	.word 0xE1600070		@ SMC
	bx	lr
ENDPROC(setup_auxcr)

.arm
ENTRY(omap3_gp_romcode_call)
	push	{r4-r12, lr}		@ Save all registers from ROM code!
	mov	r12, r0			@ Copy the Service ID in R12
	mov	r0, r1			@ Copy parameter to R0
	mcr     p15, 0, r0, c7, c10, 4	@ DSB
	mcr     p15, 0, r0, c7, c10, 5	@ DMB
	.word	0xe1600070		@ SMC #0 to enter monitor - hand assembled
					@ because we use -march=armv5
	pop	{r4-r12, pc}
ENDPROC(omap3_gp_romcode_call)

/*
 * Low-level PXA250/210 sleep/wakeUp support
 *
 * Initial SA1110 code:
 * Copyright (c) 2001 Cliff Brake <cbrake@accelent.com>
 *
 * Adapted for PXA by Nicolas Pitre:
 * Copyright (c) 2002 Monta Vista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/pxa/hardware.h>

#define MDREFR_KDIV	0x200a4000	// all banks
#define CCCR_SLEEP	0x00000107	// L=7 2N=2 A=0 PPDIS=0 CPDIS=0
#define UNCACHED_PHYS_0	0
		.text



	.ltorg
	.align	5
pxa_cpu_do_suspend:

	@ All needed values are now in registers.
	@ These last instructions should be in cache

	@ initiate the frequency change...
	str	r7, [r6]
	mcr	p14, 0, r0, c6, c0, 0

	@ restore the original cpu speed value for resume
	str	r8, [r6]

	@ need 6 13-MHz cycles before changing PWRMODE
	@ just set frequency to 91-MHz... 6*91/13 = 42

	mov	r0, #42
10:	subs	r0, r0, #1
	bne	10b

	@ Do not reorder...
	@ Intel PXA270 Specification Update notes problems performing
	@ external accesses after SDRAM is put in self-refresh mode
	@ (see Errata 39 ...hangs when entering self-refresh mode)

	@ enter sleep mode
	mcr	p14, 0, r1, c7, c0, 0		@ PWRMODE
20:	b	20b				@ loop waiting for sleep

	/*
	 * pxa3xx_finish_suspend() - forces CPU into sleep state
	*/
ENTRY(pxa3xx_suspend)
	mcr     p14, 0, r0, c7, c0, 0   @ enter sleep
20:	b       20b                     @ waiting for sleep

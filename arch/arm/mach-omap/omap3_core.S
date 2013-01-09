/**
 * @file
 * @brief Provide Architecture level Initialization
 *
 * This provides OMAP3 Architecture initialization. Among these,
 * @li OMAP ROM Code is located in SRAM, we can piggy back on
 * the same addresses
 * @li If clock initialization is required, call the same.
 * @li Setup a temporary SRAM stack which is necessary to call C
 * functions.
 * @li Call architecture initialization function a_init
 *
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
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
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/omap3-silicon.h>
#include <mach/wdt.h>
#include <mach/clocks.h>
#include <asm/barebox-arm-head.h>

#ifdef CONFIG_MACH_DO_LOWLEVEL_INIT
.section .text.__reset
ENTRY(reset)
	/* Invalidate all Dcaches */
#ifndef CONFIG_CPU_V7_DCACHE_SKIP
	/* If Arch specific ROM code SMI handling does not exist */
	mrc	p15, 1, r0, c0, c0, 1	/* read clidr */
	ands	r3, r0, #0x7000000	/* extract loc from clidr */
	mov	r3, r3, lsr #23		/* left align loc bit field */
	beq	finished_inval		/* if loc is 0, then no need to clean */
	mov	r10, #0			/* start clean at cache level 0 */
inval_loop1:
	add	r2, r10, r10, lsr #1	/* work out 3x current cache level */
	mov	r1, r0, lsr r2		/* extract cache type bits from clidr */
	and	r1, r1, #	7	/* mask of the bits for current cache only */
	cmp	r1, #2			/* see what cache we have at this level */
	blt	skip_inval		/* skip if no cache, or just i-cache */
	mcr	p15, 2, r10, c0, c0, 0	/* select current cache level in cssr */
	isb				/* isb to sych the new cssr&csidr */
	mrc	p15, 1, r1, c0, c0, 0	/* read the new csidr */
	and	r2, r1, #7		/* extract the length of the cache lines */
	add	r2, r2, #4		/* add 4 (line length offset) */
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3	/* find maximum number on the way size*/
	clz	r5, r4			/* find bit position of way size increment */
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13	/* extract max number of the index size */
inval_loop2:
	mov	r9, r4			/* create working copy of max way size */
inval_loop3:
ARM(	orr	r11, r10, r9, lsl r5	) /* factor way and cache number into r11 */
ARM(	orr	r11, r11, r7, lsl r2	) /* factor index number into r11 */
THUMB(	lsl	r6, r9, r5		)
THUMB(	orr	r11, r10, r6		) /* factor way and cache number into r11 */
THUMB(	lsl	r6, r7, r2		)
THUMB(	orr	r11, r11, r6		) /* factor index number into r11 */
	mcr	p15, 0, r11, c7, c6, 2	/* invalidate by set/way */
	subs	r9, r9, #1		/* decrement the way */
	bge	inval_loop3
	subs	r7, r7, #1		/* decrement the index */
	bge	inval_loop2
skip_inval:
	add	r10, r10, #2		/* increment cache number */
	cmp	r3, r10
	bgt	inval_loop1
finished_inval:
	mov	r10, #0			/* swith back to cache level 0 */
	mcr	p15, 2, r10, c0, c0, 0	/* select current cache level in cssr */
	isb
#endif /* CONFIG_CPU_V7_DCACHE_SKIP */
	common_reset r0
	/* back to arch calling code */
	b board_init_lowlevel_return
ENDPROC(reset)
#endif

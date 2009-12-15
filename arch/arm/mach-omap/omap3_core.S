/**
 * @file
 * @brief Provide Architecture level Initialization
 *
 * FileName: arch/arm/mach-omap/omap3_core.S
 *
 * This provides OMAP3 Architecture initialization. Among these,
 * @li OMAP ROM Code is located in SRAM, we can piggy back on
 * the same addresses
 * @li If clock initialization is required, call the same.
 * @li Setup a temporary SRAM stack which is necessary to call C
 * functions.
 * @li Call architecture initialization function a_init
 *
 * arch_init_lowlevel is enabled if CONFIG_ARCH_HAS_LOWLEVEL_INIT is defined
 * cpy_clk_code is called if CONFIG_OMAP3_COPY_CLOCK_SRAM is defined
 */
/*
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <mach/silicon.h>
#include <mach/wdt.h>
#include <mach/clocks.h>

#ifdef CONFIG_ARCH_HAS_LOWLEVEL_INIT
.globl arch_init_lowlevel
arch_init_lowlevel:
       /* Copy vectors to mask ROM indirect addr */
	mov	r0, pc			/* Store the current pc address */
	sub	r0, r0, #8		/* Reduce offset */
	ldr	r1, arch_start		/* Load the link address for arch_int */
	ldr	r2, barebox_start		/* load the link address of start_init*/
	sub	r1, r1, r2		/* get the offset */
	/* subtract the offset from PC of arch=Current start */
	sub	r0, r0, r1
	mov	r2, #OMAP_SRAM_INTVECT_COPYSIZE		/* r2 <- size to copy */
	add	r2, r0, r2		/* r2 <- source end address */
	ldr	r1, SRAM_INTVECT	/* build vect addr */
next:
	ldmia	r0!, {r3-r10}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r10}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end address [r2]    */
	ble	next			/* loop until equal */

#ifdef CONFIG_OMAP3_COPY_CLOCK_SRAM
	/* No need to copy/exec the clock code - DPLL adjust already done
	 * in Perip/NAND/oneNAND Boot.
	 * Put dpll adjust code behind vectors. r1 has address to copy to
	 */
	bl	cpy_clk_code
#endif
	ldr	r1, SRAM_INTVECT	/* build vect addr */
	/* Read the interrupt vector base address */
	mrc p15, #0, r0, c12, c0, #0
	/* Clear the vector base 4:0 is reserved. */
	and r0, r0, #0xF
	/* Store the SRAM_INTVECT address */
	orr r0, r0, r1
	/* Store the new vector address */
	mcr p15, #0, r0, c12, c0, #0
	/* Setup a temporary stack so that we can call C functions */
	ldr	sp,	SRAM_STACK
        str     ip,	[sp]    /* stash old link register */
        str     lr,	[sp]    /* stash current link register */
	mov	ip,	lr	/* save link reg across call */
	bl      a_init          /* Architecture init */
        ldr     lr,	[sp]    /* restore current link register */
        ldr     ip,	[sp]    /* restore save ip */

	/* back to arch calling code */
	mov	pc,	lr

arch_start:
	.word	arch_init_lowlevel
barebox_start:
	.word	_start
SRAM_INTVECT:
	.word	OMAP_SRAM_INTVECT

SRAM_STACK:
	.word	OMAP_SRAM_STACK
#endif /* CONFIG_ARCH_HAS_LOWLEVEL_INIT */

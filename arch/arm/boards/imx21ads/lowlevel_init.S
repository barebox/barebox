/*
 * Copyright (C) 2010 Jaccon Bastiaansen <jaccon.bastiaansen@gmail.com>
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
#include <asm-generic/memory_layout.h>
#include <mach/imx-regs.h>

	.section ".text_bare_init","ax"

.globl board_init_lowlevel
board_init_lowlevel:

/* Save lr, because it is overwritten by the calls to mem_delay. */
	mov	r10, lr

/*
 * Initialize the AHB-Lite IP Interface (AIPI) module (to enable access to
 * on chip peripherals) as described in section 7.2 of rev3 of the i.MX21
 * reference manual.
 */
	ldr	r0, =AIPI1_PSR0
	ldr	r1, =0x00040304
	str	r1, [r0]
	ldr	r0, =AIPI1_PSR1
	ldr     r1, =0xfffbfcfb
	str	r1, [r0]

	ldr	r0, =AIPI2_PSR0
	ldr	r1, =0x3ffc0000
	str	r1, [r0]
	ldr	r0, =AIPI2_PSR1
	ldr     r1, =0xffffffff
	str	r1, [r0]

/*
 * Configure CPU core clock (266MHz), peripheral clock (133MHz) and enable
 * the clock to peripherals.
 */
	ldr	r0, =CSCR
	ldr	r1, =0x17180607
	str	r1, [r0]

	ldr	r0, =PCCR1
	ldr	r1, =0x0e000000
	str	r1, [r0]


/*
 * SDRAM and SDRAM controller configuration
 */

	/*
	 * CSD1 not required, because the MX21ADS board only contains 64Mbyte.
	 * CS3 can therefore be made available.
	 */
	ldr	r0, =FMCR
	ldr	r1, =0xffffffc9
	str	r1, [r0]

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0xc0000000
	bls	1f
	cmp	pc, #0xc8000000
	bhi	1f

	mov	pc, r10
1:

	/* Precharge */
	ldr	r0, =SDCTL0
	ldr	r1, =0x92120300
	str	r1, [r0]
	ldr	r2, =0xc0200000
	ldr	r1, [r2]

	bl	mem_delay

	/* Auto refresh */
	ldr	r1, =0xa2120300
	str	r1, [r0]
	ldr	r2, =0xc0000000
	ldr	r1, [r2]
	ldr	r1, [r2]
	ldr	r1, [r2]
	ldr	r1, [r2]
	ldr	r1, [r2]
	ldr	r1, [r2]
	ldr	r1, [r2]
	ldr	r1, [r2]

	/* Set mode register */
	ldr	r1, =0xB2120300
	str	r1, [r0]
	ldr	r1, =0xC0119800
	ldr	r2, [r1]

	bl	mem_delay

	/* Back to Normal Mode */
	ldr	r1, =0x8212F339
	str	r1, [r0]

	/* Set NFC_CLK to 24MHz */
	ldr	r0, =PCDR0
	ldr	r1, =0x6419a007
	str	r1, [r0]

#ifdef CONFIG_NAND_IMX_BOOT
	ldr	sp, =STACK_BASE + STACK_SIZE - 12	/* Setup a temporary stack in SDRAM */

	ldr	r0, =IMX_NFC_BASE		/* start of NFC SRAM        */
	ldr	r2, =IMX_NFC_BASE + 0x800	/* end of NFC SRAM          */

	/* skip NAND boot if not running from NFC space */
	cmp	pc, r0
	bls	ret
	cmp	pc, r2
	bhi	ret

	/* Move ourselves out of NFC SRAM */
	ldr	r1, =_text

copy_loop:
	ldmia	r0!, {r3-r9}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r9}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end address [r2]    */
	ble	copy_loop

	ldr	pc, =1f			/* Jump to SDRAM                    */
1:
	b	nand_boot		/* Load barebox from NAND Flash     */
					/* SRAM to SDRAM                    */
#endif /* CONFIG_NAND_IMX_BOOT */

ret:
	mov	pc, r10

/*
 *  spin for a while.  we need to wait at least 200 usecs.
 */
mem_delay:
	mov	r4, #0x4000
spin:	subs	r4, r4, #1
	bne	spin
	mov	pc, lr


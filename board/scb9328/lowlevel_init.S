/*
 * Copyright (C) 2004 Sascha Hauer, Synertronixx GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#include <config.h>
#include <asm/arch/imx-regs.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

.globl board_init_lowlevel
board_init_lowlevel:

	mov	r10, lr

/* Change PERCLK1DIV to 14 ie 14+1 */
	writel(CFG_PCDR_VAL, PCDR)

/* set MCU PLL Control Register 0 */
	writel(CFG_MPCTL0_VAL, MPCTL0)

/* set mpll restart bit */
	ldr		r0, =CSCR
	ldr		r1, [r0]
	orr		r1,r1,#(1<<21)
	str		r1, [r0]

	mov		r2,#0x10
1:
	mov		r3,#0x2000
2:
	subs	r3,r3,#1
	bne		2b

	subs	r2,r2,#1
	bne		1b

/* set System PLL Control Register 0 */
	writel(CFG_SPCTL0_VAL, SPCTL0)

/* set spll restart bit */
	ldr		r0, =CSCR
	ldr		r1, [r0]
	orr		r1,r1,#(1<<22)
	str		r1, [r0]

	mov		r2,#0x10
1:
	mov		r3,#0x2000
2:
	subs	r3,r3,#1
	bne		2b

	subs	r2,r2,#1
	bne		1b

	writel(CFG_CSCR_VAL, CSCR)

/* I have now read the ARM920 DataSheet back-to-Back, and have stumbled upon
 *this.....
 *
 * It would appear that from a Cold-Boot the ARM920T enters "FastBus" mode CP15
 * register 1, this stops it using the output of the PLL and thus runs at the
 * slow rate. Unless you place the Core into "Asynch" mode, the CPU will never
 * use the value set in the CM_OSC registers...regardless of what you set it
 * too!  Thus, although i thought i was running at 140MHz, i'm actually running
 * at 40!..

 * Slapping this into my bootloader does the trick...

 * MRC p15,0,r0,c1,c0,0    ; read core configuration register
 * ORR r0,r0,#0xC0000000   ; set asynchronous clocks and not fastbus mode
 * MCR p15,0,r0,c1,c0,0    ; write modified value to core configuration
 * register
 */
	MRC p15,0,r0,c1,c0,0
	ORR r0,r0,#0xC0000000
	MCR p15,0,r0,c1,c0,0

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0x08000000
	bls	1f
	cmp	pc, #0x09000000
	bhi	1f

	mov	pc,r10

1:

/* SDRAM Setup */

	writel(0x910a8200, SDCTL0) /* Precharge cmd, CAS = 2 */
	writel(0x0, 0x08200000) /* Issue Precharge all Command */
	writel(0xa10a8200, SDCTL0) /* Autorefresh cmd, CAS = 2 */

	ldr		r0, =0x08000000
	ldr		r1, =0x0 /* Issue AutoRefresh Command */
	str		r1,   [r0]
	str		r1,   [r0]
	str		r1,   [r0]
	str		r1,   [r0]
	str		r1,   [r0]
	str		r1,   [r0]
	str		r1,   [r0]
	str		r1,   [r0]

	writel(0xb10a8300, SDCTL0)
	writel(0x0, 0x08223000) /* CAS Latency 2, issue Mode Register Command, Burst Length = 8 */
	writel(0x810a8200, SDCTL0) /* Set to Normal Mode CAS 2 */

	mov	pc,r10

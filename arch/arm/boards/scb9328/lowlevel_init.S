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
 */

#include <mach/imx1-regs.h>
#include <asm/barebox-arm-head.h>

#define CPU200

#ifdef CPU200
#define CFG_MPCTL0_VAL 0x00321431
#else
#define CFG_MPCTL0_VAL 0x040e200e
#endif

#define BUS72

#ifdef BUS72
#define CFG_SPCTL0_VAL 0x04002400
#endif

#ifdef BUS96
#define CFG_SPCTL0_VAL 0x04001800
#endif

#ifdef BUS64
#define CFG_SPCTL0_VAL 0x08001800
#endif

/* Das ist der BCLK Divider, der aus der System PLL
   BCLK und HCLK erzeugt:
   31 | xxxx xxxx xxxx xxxx xx10 11xx xxxx xxxx | 0
   0x2f008403 : 192MHz/2=96MHz, 144MHz/2=72MHz PRESC=1->BCLKDIV=2
   0x2f008803 : 192MHz/3=64MHz, 240MHz/3=80MHz PRESC=1->BCLKDIV=2
   0x2f001003 : 192MHz/5=38,4MHz
   0x2f000003 : 64MHz/1
   Bit 22: SPLL Restart
   Bit 21: MPLL Restart */

#ifdef BUS64
#define CFG_CSCR_VAL 0x2f030003
#endif

#ifdef BUS72
#define CFG_CSCR_VAL 0x2f030403
#endif
/* Bit[0:3] contain PERCLK1DIV for UART 1
   0x000b00b ->b<- -> 192MHz/12=16MHz
   0x000b00b ->8<- -> 144MHz/09=16MHz
   0x000b00b ->3<- -> 64MHz/4=16MHz */

#ifdef BUS96
#define CFG_PCDR_VAL 0x000b00b5
#endif

#ifdef BUS64
#define CFG_PCDR_VAL 0x000b00b3
#endif

#ifdef BUS72
#define CFG_PCDR_VAL 0x000b00b8
#endif

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:

	arm_cpu_lowlevel_init r0

	/* Change PERCLK1DIV to 14 ie 14+1 */
	writel(CFG_PCDR_VAL, MX1_CCM_BASE_ADDR + MX1_PCDR)

	/* set MCU PLL Control Register 0 */
	writel(CFG_MPCTL0_VAL, MX1_CCM_BASE_ADDR + MX1_MPCTL0)

	/* set mpll restart bit */
	ldr		r0, =MX1_CCM_BASE_ADDR + MX1_CSCR
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
	writel(CFG_SPCTL0_VAL, MX1_CCM_BASE_ADDR + MX1_SPCTL0)

	/* set spll restart bit */
	ldr		r0, =MX1_CCM_BASE_ADDR + MX1_CSCR
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

	writel(CFG_CSCR_VAL, MX1_CCM_BASE_ADDR + MX1_CSCR)

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

	b imx1_barebox_entry

1:

/* SDRAM Setup */

	/* Precharge cmd, CAS = 2 */
	writel(0x910a8200, MX1_SDRAMC_BASE_ADDR + MX1_SDCTL0)
	/* Issue Precharge all Command */
	writel(0x0, 0x08200000)
	/* Autorefresh cmd, CAS = 2 */
	writel(0xa10a8200, MX1_SDRAMC_BASE_ADDR + MX1_SDCTL0)

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

	writel(0xb10a8300, MX1_SDRAMC_BASE_ADDR + MX1_SDCTL0)
	/* CAS Latency 2, issue Mode Register Command, Burst Length = 8 */
	writel(0x0, 0x08223000)
	/* Set to Normal Mode CAS 2 */
	writel(0x810a8200, MX1_SDRAMC_BASE_ADDR + MX1_SDCTL0)

	b imx1_barebox_entry

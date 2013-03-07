/*
 * barebox - start.S Startup file of barebox for BF533/BF561
 *
 * Copyright (c) 2005 blackfin.uclinux.org
 *
 * This file is based on head.S
 * Copyright (c) 2003  Metrowerks/Motorola
 * Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 *                     The Silver Hammer Group, Ltd.
 * (c) 1995, Dionne & Associates
 * (c) 1995, DKG Display Tech.
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

#include <config.h>
#include <asm/blackfin.h>
#include <asm/cpu/defBF561_extn.h>
#include <asm/cpu/defBF561.h>
#include <asm-generic/memory_layout.h>

.section ".text_entry","ax"

.macro checker
	p0.h = FIO0_DIR >> 16;
	p0.l = FIO0_DIR & 0xFFFF;
	r0 = (1 << 9);
	w[p0] = r0;
	p0.h = FIO0_FLAG_S >> 16;
	p0.l = FIO0_FLAG_S & 0xFFFF;
	r0 = (1 << 9);
	w[p0] = r0;
1:
	jump 1b
.endm

_start:
start:
_stext:

	R0 = 0x32;
	SYSCFG = R0;
	SSYNC;

	/* As per HW reference manual DAG registers,
	 * DATA and Address resgister shall be zero'd 
	 * in initialization, after a reset state
	 */
	r1 = 0;	/* Data registers zero'd */
	r2 = 0;
	r3 = 0;
	r4 = 0;
	r5 = 0;
	r6 = 0;
	r7 = 0;

	p0 = 0; /* Address registers zero'd */
	p1 = 0;
	p2 = 0;
	p3 = 0;
	p4 = 0;
	p5 = 0;
	
	i0 = 0; /* DAG Registers zero'd */
	i1 = 0;
	i2 = 0;
	i3 = 0;
	m0 = 0;
	m1 = 0;
	m3 = 0;
	m3 = 0;
	l0 = 0;
	l1 = 0;
	l2 = 0;
	l3 = 0;
	b0 = 0;
	b1 = 0;
	b2 = 0;
	b3 = 0;

	/* Set loop counters to zero, to make sure that
	 * hw loops are disabled.
	 */
	r0  = 0;
	lc0 = r0;
	lc1 = r0;

	SSYNC;

	/* Check soft reset status */
	p0.h = SWRST >> 16;
	p0.l = SWRST & 0xFFFF;
	r0.l = w[p0];

	cc = bittst(r0, 15);
	if !cc jump no_soft_reset;

	/* Clear Soft reset */
	r0 = 0x0000;
	w[p0] = r0;
	ssync;

no_soft_reset:
	nop;

	/* Clear EVT registers */
	p0.h = hi(EVT_EMULATION_ADDR)
	p0.l = lo(EVT_EMULATION_ADDR);
	p0 += 8;
	p1 = 14;
	r1 = 0;
	LSETUP(4,4) lc0 = p1;
	[ p0 ++ ] = r1;
	
	p0.h = hi(SIC_IWR);
        p0.l = lo(SIC_IWR);
        r0.l = 0x1;
        w[p0] = r0.l;
        SSYNC;

	sp.l = lo(0xffb01000);
	sp.h = hi(0xffb01000);

	/* Check if the code is in SDRAM */
	/* If the code is in SDRAM, skip SDRAM initializaiton */
	call get_pc;
	r3.l = 0x0;
	r3.h = 0x2000;
	cc = r0 < r3 (iu);
	if cc jump sdram_initialized;
	call init_sdram;
	/* relocate into to RAM */
sdram_initialized:
	call get_pc;
offset:
	r2.l = offset;
	r2.h = offset;
	r3.l = start;
	r3.h = start;
	r1 = r2 - r3;

	r0 = r0 - r1;
	p1 = r0;

	p2.l = lo(TEXT_BASE);
	p2.h = hi(TEXT_BASE);

	p3 = 0x04;
	p4.l = ___bss_start;
	p4.h = ___bss_start;
loop1:
	r1 = [p1 ++ p3];
	[p2 ++ p3] = r1;
	cc=p2==p4;
	if !cc jump loop1;

	/*
	 * configure STACK
	 */

	r0.h = hi(STACK_BASE);
	r0.l = lo(STACK_BASE);
	sp = r0;
	fp = sp;

	/*
	 * This next section keeps the processor in supervisor mode
	 * during kernel boot.  Switches to user mode at end of boot.
	 * See page 3-9 of Hardware Reference manual for documentation.
	 */

	/* To keep ourselves in the supervisor mode */
	p0.l = lo(EVT_IVG15_ADDR);
	p0.h = hi(EVT_IVG15_ADDR);

	p1.l = _real_start;
	p1.h = _real_start;
	[p0] = p1;

	p0.l = lo(IMASK);
	p0.h = hi(IMASK);
	r0.l = lo(IVG15_POS);
	r0.h = hi(IVG15_POS);
	[p0] = r0;
	raise 15;
	p0.l = WAIT_HERE;
	p0.h = WAIT_HERE;
	reti = p0;
	rti;

WAIT_HERE:
	jump WAIT_HERE;

.global _real_start;
_real_start:
	[ -- sp ] = reti;

#if defined(CONFIG_EZKIT533) || defined(CONFIG_EZKIT561)
	p0.l = lo(WDOG_CTL);
	p0.h = hi(WDOG_CTL);
	r0 = WATCHDOG_DISABLE(z);
	w[p0] = r0;
#endif


#ifdef CONFIG_BF537
/* Initialise General-Purpose I/O Modules on BF537 
 * Rev 0.0 Anomaly 05000212 - PORTx_FER, 
 * PORT_MUX Registers Do Not accept "writes" correctly
 */
        p0.h = hi(PORTF_FER);
        p0.l = lo(PORTF_FER);
        R0.L = W[P0]; /* Read */
	nop;
	nop;
	nop;
        ssync;
        R0 = 0x000F(Z);
        W[P0] = R0.L; /* Write */
	nop;
	nop;
	nop;
        ssync;
        W[P0] = R0.L; /* Enable peripheral function of PORTF for UART0 and UART1 */
	nop;
	nop;
	nop;
        ssync;

 	p0.h = hi(PORTH_FER);
        p0.l = lo(PORTH_FER);
        R0.L = W[P0]; /* Read */
        nop;
        nop;
        nop;
        ssync;
        R0 = 0xFFFF(Z);
        W[P0] = R0.L; /* Write */
        nop;
        nop;
        nop;
        ssync;
        W[P0] = R0.L; /* Enable peripheral function of PORTH for MAC */
        nop;
        nop;
        nop;
        ssync;

#endif

	/* DMA reset code to Hi of L1 SRAM */
copy:
	P1.H = hi(SYSMMR_BASE);	/* P1 Points to the beginning of SYSTEM MMR Space */
	P1.L = lo(SYSMMR_BASE);

	R0.H = reset_start;	/* Source Address (high) */
	R0.L = reset_start;	/* Source Address (low) */
	R1.H = reset_end;
	R1.L = reset_end;
	R2 = R1 - R0;		/* Count */
	R1.H = hi(L1_ISRAM);	/* Destination Address (high) */
	R1.L = lo(L1_ISRAM);	/* Destination Address (low) */
	R3.L = DMAEN;		/* Source DMAConfig Value (8-bit words) */
	R4.L = (DI_EN | WNR | DMAEN);	/* Destination DMAConfig Value (8-bit words) */

DMA:
	R6 = 0x1 (Z);
	W[P1+OFFSET_(MDMA_S0_X_MODIFY)] = R6;	/* Source Modify = 1 */
	W[P1+OFFSET_(MDMA_D0_X_MODIFY)] = R6;	/* Destination Modify = 1 */

	[P1+OFFSET_(MDMA_S0_START_ADDR)] = R0;	/* Set Source Base Address */
	W[P1+OFFSET_(MDMA_S0_X_COUNT)] = R2;	/* Set Source Count */
	/* Set Source  DMAConfig = DMA Enable,
	Memory Read,  8-Bit Transfers, 1-D DMA, Flow - Stop */
	W[P1+OFFSET_(MDMA_S0_CONFIG)] = R3;

	[P1+OFFSET_(MDMA_D0_START_ADDR)] = R1;	/* Set Destination Base Address */
	W[P1+OFFSET_(MDMA_D0_X_COUNT)] = R2;	/* Set Destination Count */
	/* Set Destination DMAConfig = DMA Enable,
	Memory Write, 8-Bit Transfers, 1-D DMA, Flow - Stop, IOC */
	W[P1+OFFSET_(MDMA_D0_CONFIG)] = R4;
	
WAIT_DMA_DONE:	
	p0.h = hi(MDMA_D0_IRQ_STATUS);
	p0.l = lo(MDMA_D0_IRQ_STATUS);
	R0 = W[P0](Z);
	CC = BITTST(R0, 0);
	if ! CC jump WAIT_DMA_DONE

	R0 = 0x1;
	W[P1+OFFSET_(MDMA_D0_IRQ_STATUS)] = R0;	/* Write 1 to clear DMA interrupt */

	/* Initialize BSS Section with 0 s */
	p1.l = ___bss_start;
	p1.h = ___bss_start;
	p2.l = _end;
	p2.h = _end;
	r1 = p1;
	r2 = p2;
	r3 = r2 - r1;
	r3 = r3 >> 2;
	p3 = r3;
	lsetup (_clear_bss, _clear_bss_end ) lc1 = p3;
	CC = p2<=p1;
	if CC jump _clear_bss_skip;
	r0 = 0;
_clear_bss:
_clear_bss_end:
 	[p1++] = r0;
_clear_bss_skip:

#if defined(CONFIG_BF537)&&defined(CONFIG_POST)
        p0.l = post_flag;
        p0.h = post_flag;
        r0   = r7;
        [p0] = r0;
#endif

	p0.l = _blackfin_start_barebox;
	p0.h = _blackfin_start_barebox;
	jump (p0);

reset_start:
	p0.h = hi(WDOG_CNT);
	p0.l = lo(WDOG_CNT);
	r0 = 0x0010;
	w[p0] = r0;
	p0.h = hi(WDOG_CTL);
	p0.l = lo(WDOG_CTL);
	r0 = 0x0000;
	w[p0] = r0;
reset_wait:
	jump reset_wait;

reset_end: nop;

_exit:
	jump.s	_exit;
get_pc:
	r0 = rets;
	rts;

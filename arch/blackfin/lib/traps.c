/*
 * barebox - traps.c Routines related to interrupts and exceptions
 *
 * Copyright (c) 2005 blackfin.uclinux.org
 *
 * This file is based on
 * No original Copyright holder listed,
 * Probabily original (C) Roman Zippel (assigned DJD, 1999)
 *
 * Copyright 2003 Metrowerks - for Blackfin
 * Copyright 2000-2001 Lineo, Inc. D. Jeff Dionne <jeff@lineo.ca>
 * Copyright 1999-2000 D. Jeff Dionne, <jeff@uclinux.org>
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#include <common.h>
#include <linux/types.h>
#include <asm/system.h>
#include <asm/traps.h>
#include <asm/page.h>
#include <asm/cplb.h>
#include <asm/ptrace.h>
#include <asm/cpu.h>

void dump_regs(struct pt_regs *fp)
{
	printf("DCPLB_FAULT_ADDR=%p\n", *pDCPLB_FAULT_ADDR);
	printf("ICPLB_FAULT_ADDR=%p\n", *pICPLB_FAULT_ADDR);

	printf("stack frame=0x%x, ", (unsigned int) fp);
	printf("bad PC=0x%04x\n", (unsigned int) fp->pc);
        printf("RETE:  %08lx  RETN: %08lx  RETX: %08lx  RETS: %08lx\n", fp->rete, fp->retn, fp->retx, fp->rets);
        printf("IPEND: %04lx  SYSCFG: %04lx\n", fp->ipend, fp->syscfg);
        printf("SEQSTAT: %08lx    SP: %08lx\n", (long)fp->seqstat, (long)fp);
        printf("R0: %08lx    R1: %08lx    R2: %08lx    R3: %08lx\n", fp->r0, fp->r1, fp->r2, fp->r3);
        printf("R4: %08lx    R5: %08lx    R6: %08lx    R7: %08lx\n", fp->r4, fp->r5, fp->r6, fp->r7);
        printf("P0: %08lx    P1: %08lx    P2: %08lx    P3: %08lx\n", fp->p0, fp->p1, fp->p2, fp->p3);
        printf("P4: %08lx    P5: %08lx    FP: %08lx\n", fp->p4, fp->p5, fp->fp);
        printf("A0.w: %08lx    A0.x: %08lx    A1.w: %08lx    A1.x: %08lx\n", fp->a0w, fp->a0x, fp->a1w, fp->a1x);

        printf("LB0: %08lx  LT0: %08lx  LC0: %08lx\n", fp->lb0, fp->lt0, fp->lc0);
        printf("LB1: %08lx  LT1: %08lx  LC1: %08lx\n", fp->lb1, fp->lt1, fp->lc1);
        printf("B0: %08lx  L0: %08lx  M0: %08lx  I0: %08lx\n", fp->b0, fp->l0, fp->m0, fp->i0);
        printf("B1: %08lx  L1: %08lx  M1: %08lx  I1: %08lx\n", fp->b1, fp->l1, fp->m1, fp->i1);
        printf("B2: %08lx  L2: %08lx  M2: %08lx  I2: %08lx\n", fp->b2, fp->l2, fp->m2, fp->i2);
        printf("B3: %08lx  L3: %08lx  M3: %08lx  I3: %08lx\n", fp->b3, fp->l3, fp->m3, fp->i3);
}

static const char *trap_to_string(int trapnr)
{
	switch (trapnr) {
	case VEC_MISALI_D:
		return "Data access misaligned";
	case VEC_MISALI_I:
		return "Instruction fetch misaligned";
	case VEC_CPLB_I_M:
		return "Instruction fetch CPLB miss";
	}
	return NULL;
}

void trap_c (struct pt_regs *regs)
{
	uint32_t trapnr = (regs->seqstat) & SEQSTAT_EXCAUSE;
	const char *str;

	printf("Exception occured!\n\n");

	str = trap_to_string(trapnr);
	if (str)
		printf("%s\n", str);
	printf("code=[0x%x]\n", trapnr);

	dump_regs(regs);

	printf("\nPlease reset the board\n");

	reset_cpu(0);
}

void blackfin_irq_panic(int reason, struct pt_regs *regs)
{
	printf("\n\nException: IRQ 0x%x entered\n", reason);
	dump_regs(regs);
	printf("Unhandled IRQ or exceptions!\n");
	printf("Please reset the board \n");

	reset_cpu(0);
}


/*
 * interrupts_64.c - Interrupt Support Routines
 *
 * Copyright (c) 2018 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 *
 */

#include <common.h>
#include <abort.h>
#include <asm/ptrace.h>
#include <asm/unwind.h>
#include <init.h>
#include <asm/system.h>
#include <asm/esr.h>

/* Avoid missing prototype warning, called from assembly */
void do_bad_sync (struct pt_regs *pt_regs);
void do_bad_irq (struct pt_regs *pt_regs);
void do_bad_fiq (struct pt_regs *pt_regs);
void do_bad_error (struct pt_regs *pt_regs);
void do_fiq (struct pt_regs *pt_regs);
void do_irq (struct pt_regs *pt_regs);
void do_error (struct pt_regs *pt_regs);
void do_sync(struct pt_regs *pt_regs, unsigned int esr, unsigned long far);

static const char *esr_class_str[] = {
	[0 ... ESR_ELx_EC_MAX]		= "UNRECOGNIZED EC",
	[ESR_ELx_EC_UNKNOWN]		= "Unknown/Uncategorized",
	[ESR_ELx_EC_WFx]		= "WFI/WFE",
	[ESR_ELx_EC_CP15_32]		= "CP15 MCR/MRC",
	[ESR_ELx_EC_CP15_64]		= "CP15 MCRR/MRRC",
	[ESR_ELx_EC_CP14_MR]		= "CP14 MCR/MRC",
	[ESR_ELx_EC_CP14_LS]		= "CP14 LDC/STC",
	[ESR_ELx_EC_FP_ASIMD]		= "ASIMD",
	[ESR_ELx_EC_CP10_ID]		= "CP10 MRC/VMRS",
	[ESR_ELx_EC_CP14_64]		= "CP14 MCRR/MRRC",
	[ESR_ELx_EC_ILL]		= "PSTATE.IL",
	[ESR_ELx_EC_SVC32]		= "SVC (AArch32)",
	[ESR_ELx_EC_HVC32]		= "HVC (AArch32)",
	[ESR_ELx_EC_SMC32]		= "SMC (AArch32)",
	[ESR_ELx_EC_SVC64]		= "SVC (AArch64)",
	[ESR_ELx_EC_HVC64]		= "HVC (AArch64)",
	[ESR_ELx_EC_SMC64]		= "SMC (AArch64)",
	[ESR_ELx_EC_SYS64]		= "MSR/MRS (AArch64)",
	[ESR_ELx_EC_IMP_DEF]		= "EL3 IMP DEF",
	[ESR_ELx_EC_IABT_LOW]		= "IABT (lower EL)",
	[ESR_ELx_EC_IABT_CUR]		= "IABT (current EL)",
	[ESR_ELx_EC_PC_ALIGN]		= "PC Alignment",
	[ESR_ELx_EC_DABT_LOW]		= "DABT (lower EL)",
	[ESR_ELx_EC_DABT_CUR]		= "DABT (current EL)",
	[ESR_ELx_EC_SP_ALIGN]		= "SP Alignment",
	[ESR_ELx_EC_FP_EXC32]		= "FP (AArch32)",
	[ESR_ELx_EC_FP_EXC64]		= "FP (AArch64)",
	[ESR_ELx_EC_SERROR]		= "SError",
	[ESR_ELx_EC_BREAKPT_LOW]	= "Breakpoint (lower EL)",
	[ESR_ELx_EC_BREAKPT_CUR]	= "Breakpoint (current EL)",
	[ESR_ELx_EC_SOFTSTP_LOW]	= "Software Step (lower EL)",
	[ESR_ELx_EC_SOFTSTP_CUR]	= "Software Step (current EL)",
	[ESR_ELx_EC_WATCHPT_LOW]	= "Watchpoint (lower EL)",
	[ESR_ELx_EC_WATCHPT_CUR]	= "Watchpoint (current EL)",
	[ESR_ELx_EC_BKPT32]		= "BKPT (AArch32)",
	[ESR_ELx_EC_VECTOR32]		= "Vector catch (AArch32)",
	[ESR_ELx_EC_BRK64]		= "BRK (AArch64)",
};

const char *esr_get_class_string(u32 esr)
{
	return esr_class_str[esr >> ESR_ELx_EC_SHIFT];
}

/**
 * Display current register set content
 * @param[in] regs Guess what
 */
void show_regs(struct pt_regs *regs)
{
	int i;

	printf("elr: %016lx lr : %016lx\n", regs->elr, regs->regs[30]);

	for (i = 0; i < 29; i += 2)
		printf("x%-2d: %016lx x%-2d: %016lx\n",
			i, regs->regs[i], i + 1, regs->regs[i + 1]);
	printf("\n");
}

static void __noreturn do_exception(struct pt_regs *pt_regs)
{
	show_regs(pt_regs);

	unwind_backtrace(pt_regs);

	panic("panic: unhandled exception");
}

/**
 * The CPU catches a fast interrupt request.
 * @param[in] pt_regs Register set content when the interrupt happens
 *
 * We never enable FIQs, so this should not happen
 */
void do_fiq(struct pt_regs *pt_regs)
{
	printf ("fast interrupt request\n");
	do_exception(pt_regs);
}

/**
 * The CPU catches a regular interrupt.
 * @param[in] pt_regs Register set content when the interrupt happens
 *
 * We never enable interrupts, so this should not happen
 */
void do_irq(struct pt_regs *pt_regs)
{
	printf ("interrupt request\n");
	do_exception(pt_regs);
}

void do_bad_sync(struct pt_regs *pt_regs)
{
	printf("bad sync\n");
	do_exception(pt_regs);
}

void do_bad_irq(struct pt_regs *pt_regs)
{
	printf("bad irq\n");
	do_exception(pt_regs);
}

void do_bad_fiq(struct pt_regs *pt_regs)
{
	printf("bad fiq\n");
	do_exception(pt_regs);
}

void do_bad_error(struct pt_regs *pt_regs)
{
	printf("bad error\n");
	do_exception(pt_regs);
}

extern volatile int arm_ignore_data_abort;
extern volatile int arm_data_abort_occurred;

void do_sync(struct pt_regs *pt_regs, unsigned int esr, unsigned long far)
{
	if ((esr >> ESR_ELx_EC_SHIFT) == ESR_ELx_EC_DABT_CUR &&
			arm_ignore_data_abort) {
		arm_data_abort_occurred = 1;
		pt_regs->elr += 4;
		return;
	}

	printf("%s exception (ESR 0x%08x) at 0x%016lx\n", esr_get_class_string(esr),
	       esr, far);
	do_exception(pt_regs);
}


void do_error(struct pt_regs *pt_regs)
{
	printf("error exception\n");
	do_exception(pt_regs);
}

void data_abort_mask(void)
{
	arm_data_abort_occurred = 0;
	arm_ignore_data_abort = 1;
}

int data_abort_unmask(void)
{
	arm_ignore_data_abort = 0;

	return arm_data_abort_occurred != 0;
}

extern unsigned long vectors;

static int aarch64_init_vectors(void)
{
        unsigned int el;

        el = current_el();
        if (el == 1)
                asm volatile("msr vbar_el1, %0" : : "r" (&vectors) : "cc");
        else if (el == 2)
                asm volatile("msr vbar_el2, %0" : : "r" (&vectors) : "cc");
        else
                asm volatile("msr vbar_el3, %0" : : "r" (&vectors) : "cc");

	return 0;
}
pure_initcall(aarch64_init_vectors);

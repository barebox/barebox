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

/**
 * Display current register set content
 * @param[in] regs Guess what
 */
void show_regs(struct pt_regs *regs)
{
}

static void __noreturn do_exception(struct pt_regs *pt_regs)
{
	show_regs(pt_regs);

	panic("");
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

void do_sync(struct pt_regs *pt_regs)
{
	printf("sync exception\n");
	do_exception(pt_regs);
}


void do_error(struct pt_regs *pt_regs)
{
	printf("error exception\n");
	do_exception(pt_regs);
}

extern volatile int arm_ignore_data_abort;
extern volatile int arm_data_abort_occurred;

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

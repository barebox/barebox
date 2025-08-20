// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2008 ARM Limited
 * Copyright (C) 2014 Regents of the University of California
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 *
 * Framepointer assisted stack unwinder
 */

#include <linux/kernel.h>
#include <printf.h>
#include <stdio.h>
#include <asm/unwind.h>
#include <asm/ptrace.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

struct stackframe {
	unsigned long fp;
	unsigned long ra;
};

static void dump_backtrace_entry(unsigned long where, unsigned long from)
{
#ifdef CONFIG_KALLSYMS
	eprintf("[<%08lx>] (%pS) from [<%08lx>] (%pS)\n", where, (void *)where, from, (void *)from);
#else
	eprintf("Function entered at [<%08lx>] from [<%08lx>]\n", where, from);
#endif
}

static int unwind_frame(struct stackframe *frame, unsigned long *sp)
{
	unsigned long low, high;
	unsigned long fp = frame->fp;

	low = *sp;
	high = ALIGN(low, STACK_SIZE);

	if (fp < low || fp > high - sizeof(struct stackframe) || fp & 0x7)
		return -1;

	*frame = *((struct stackframe *)fp - 1);
	*sp = fp;

	return 0;
}

void unwind_backtrace(const struct pt_regs *regs)
{
        struct stackframe frame = {};
	register unsigned long current_sp asm ("sp");
	unsigned long sp = 0;

	if (regs) {
		frame.fp = frame_pointer(regs);
		frame.ra = regs->ra;
	} else {
		sp = current_sp;
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.ra = (unsigned long)unwind_backtrace;
	}

	eprintf("Call trace:\n");
	for (;;) {
		unsigned long where = frame.ra;
		int ret;

		ret = unwind_frame(&frame, &sp);
		if (ret < 0)
			break;

		dump_backtrace_entry(where, frame.ra);
	}
}

void dump_stack(void)
{
	unwind_backtrace(NULL);
}

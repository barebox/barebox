/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <asm/stacktrace.h>
#include <asm/unwind.h>

#define THREAD_SIZE 16384

/*
 * AArch64 PCS assigns the frame pointer to x29.
 *
 * A simple function prologue looks like this:
 * 	sub	sp, sp, #0x10
 *   	stp	x29, x30, [sp]
 *	mov	x29, sp
 *
 * A simple function epilogue looks like this:
 *	mov	sp, x29
 *	ldp	x29, x30, [sp]
 *	add	sp, sp, #0x10
 */
int unwind_frame(struct stackframe *frame)
{
	unsigned long high, low;
	unsigned long fp = frame->fp;

	low  = frame->sp;
	high = ALIGN(low, THREAD_SIZE);

	if (fp < low || fp > high - 0x18 || fp & 0xf)
		return -EINVAL;

	frame->sp = fp + 0x10;
	frame->fp = *(unsigned long *)(fp);
	frame->pc = *(unsigned long *)(fp + 8);

	return 0;
}

static void dump_backtrace_entry(unsigned long where, unsigned long from)
{
#ifdef CONFIG_KALLSYMS
	printf("[<%08lx>] (%pS) from [<%08lx>] (%pS)\n", where, (void *)where, from, (void *)from);
#else
	printf("Function entered at [<%08lx>] from [<%08lx>]\n", where, from);
#endif
}

void unwind_backtrace(struct pt_regs *regs)
{
        struct stackframe frame = {};
	register unsigned long current_sp asm ("sp");

	if (regs) {
		frame.fp = regs->regs[29];
		frame.pc = regs->elr;
	} else {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_sp;
		frame.pc = (unsigned long)unwind_backtrace;
	}

	printf("Call trace:\n");
	while (1) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(&frame);
		if (ret < 0)
			break;
		dump_backtrace_entry(where, frame.pc);
	}
}

void dump_stack(void)
{
	unwind_backtrace(NULL);
}

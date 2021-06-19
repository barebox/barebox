// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016-17 Microsemi Corporation.
 * Padmarao Begari, Microsemi Corporation <padmarao.begari@microsemi.com>
 *
 * Copyright (C) 2017 Andes Technology Corporation
 * Rick Chen, Andes Technology Corporation <rick@andestech.com>
 *
 * Copyright (C) 2019 Sean Anderson <seanga2@gmail.com>
 */

#include <common.h>
#include <asm/system.h>
#include <asm/ptrace.h>
#include <asm/irq.h>
#include <asm/csr.h>
#include <abort.h>
#include <pbl.h>

#define MCAUSE32_INT	0x80000000
#define MCAUSE64_INT	0x8000000000000000

#ifdef CONFIG_64BIT
# define MCAUSE_INT MCAUSE64_INT
#else
# define MCAUSE_INT MCAUSE32_INT
#endif

static void show_regs(const struct pt_regs *regs)
{
	printf("\nsp:  " REG_FMT " gp:  " REG_FMT " tp:  " REG_FMT "\n",
	       regs->sp, regs->gp, regs->tp);
	printf("t0:  " REG_FMT " t1:  " REG_FMT " t2:  " REG_FMT "\n",
	       regs->t0, regs->t1, regs->t2);
	printf("s0:  " REG_FMT " s1:  " REG_FMT " a0:  " REG_FMT "\n",
	       regs->s0, regs->s1, regs->a0);
	printf("a1:  " REG_FMT " a2:  " REG_FMT " a3:  " REG_FMT "\n",
	       regs->a1, regs->a2, regs->a3);
	printf("a4:  " REG_FMT " a5:  " REG_FMT " a6:  " REG_FMT "\n",
	       regs->a4, regs->a5, regs->a6);
	printf("a7:  " REG_FMT " s2:  " REG_FMT " s3:  " REG_FMT "\n",
	       regs->a7, regs->s2, regs->s3);
	printf("s4:  " REG_FMT " s5:  " REG_FMT " s6:  " REG_FMT "\n",
	       regs->s4, regs->s5, regs->s6);
	printf("s7:  " REG_FMT " s8:  " REG_FMT " s9:  " REG_FMT "\n",
	       regs->s7, regs->s8, regs->s9);
	printf("s10: " REG_FMT " s11: " REG_FMT " t3:  " REG_FMT "\n",
	       regs->s10, regs->s11, regs->t3);
	printf("t4:  " REG_FMT " t5:  " REG_FMT " t6:  " REG_FMT "\n",
	       regs->t4, regs->t5, regs->t6);
}

static void report_trap(const struct pt_regs *regs)
{
	static const char * const exception_code[] = {
		[0]  = "Instruction address misaligned",
		[1]  = "Instruction access fault",
		[2]  = "Illegal instruction",
		[3]  = "Breakpoint",
		[4]  = "Load address misaligned",
		[5]  = "Load access fault",
		[6]  = "Store/AMO address misaligned",
		[7]  = "Store/AMO access fault",
		[8]  = "Environment call from U-mode",
		[9]  = "Environment call from S-mode",
		[10] = "Reserved",
		[11] = "Environment call from M-mode",
		[12] = "Instruction page fault",
		[13] = "Load page fault",
		[14] = "Reserved",
		[15] = "Store/AMO page fault",

	};

	printf("Unhandled exception: %ld", regs->cause);

	if (regs->cause < ARRAY_SIZE(exception_code))
		printf(" \"%s\"\n", exception_code[regs->cause]);

	printf("E [<" REG_FMT ">] ra: [<" REG_FMT ">] tval: " REG_FMT "\n",
	       regs->epc, regs->ra, regs->badaddr);

	show_regs(regs);
}



#ifdef __PBL__

static inline bool skip_data_abort(struct pt_regs *regs)
{
	return false;
}

#else

static volatile bool riscv_data_abort_occurred;
static volatile bool riscv_ignore_data_abort;

void data_abort_mask(void)
{
	riscv_data_abort_occurred = false;
	riscv_ignore_data_abort = true;
}

int data_abort_unmask(void)
{
	riscv_ignore_data_abort = false;
	return riscv_data_abort_occurred;
}

static inline bool skip_data_abort(struct pt_regs *regs)
{
	return regs->cause == EXC_LOAD_ACCESS && riscv_ignore_data_abort;
}

#endif

unsigned long handle_trap(struct pt_regs *regs)
{
	if (skip_data_abort(regs))
		goto skip;

	if (regs->cause == 2) { /* illegal instruction */
		switch(*(unsigned long *)regs->epc) {
		case 0x0000100f: /* fence.i */
			goto skip;
		default:
			break;
		}
	}

	report_trap(regs);
	hang();

skip:
	return regs->epc + 4;
}


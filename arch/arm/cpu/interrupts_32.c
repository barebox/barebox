// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/**
 * @file
 * @brief Interrupt Support Routines
 */

#include <common.h>
#include <abort.h>
#include <linux/sizes.h>
#include <asm/ptrace.h>
#include <asm/barebox-arm.h>
#include <asm/unwind.h>
#include <asm/system_info.h>
#include <init.h>

#include "mmu_32.h"

/* Avoid missing prototype warning, called from assembly */
void do_undefined_instruction (struct pt_regs *pt_regs);
void do_software_interrupt (struct pt_regs *pt_regs);
void do_prefetch_abort (struct pt_regs *pt_regs);
void do_data_abort (struct pt_regs *pt_regs);
void do_fiq (struct pt_regs *pt_regs);
void do_irq (struct pt_regs *pt_regs);

/**
 * Display current register set content
 * @param[in] regs Guess what
 */
void show_regs (struct pt_regs *regs)
{
	unsigned long flags;
	const char *processor_modes[] = {
	"USER_26",	"FIQ_26",	"IRQ_26",	"SVC_26",
	"UK4_26",	"UK5_26",	"UK6_26",	"UK7_26",
	"UK8_26",	"UK9_26",	"UK10_26",	"UK11_26",
	"UK12_26",	"UK13_26",	"UK14_26",	"UK15_26",
	"USER_32",	"FIQ_32",	"IRQ_32",	"SVC_32",
	"UK4_32",	"UK5_32",	"UK6_32",	"ABT_32",
	"UK8_32",	"UK9_32",	"UK10_32",	"UND_32",
	"UK12_32",	"UK13_32",	"UK14_32",	"SYS_32",
	};

	flags = condition_codes (regs);

	eprintf("pc : [<%08lx>]    lr : [<%08lx>]\n"
		"sp : %08lx  ip : %08lx  fp : %08lx\n",
		instruction_pointer (regs),
		regs->ARM_lr, regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	eprintf("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9, regs->ARM_r8);
	eprintf("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6, regs->ARM_r5, regs->ARM_r4);
	eprintf("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2, regs->ARM_r1, regs->ARM_r0);
	eprintf("Flags: %c%c%c%c",
		flags & PSR_N_BIT ? 'N' : 'n',
		flags & PSR_Z_BIT ? 'Z' : 'z',
		flags & PSR_C_BIT ? 'C' : 'c', flags & PSR_V_BIT ? 'V' : 'v');
	eprintf("  IRQs %s  FIQs %s  Mode %s%s\n",
		interrupts_enabled (regs) ? "on" : "off",
		fast_interrupts_enabled (regs) ? "on" : "off",
		processor_modes[processor_mode (regs)],
		thumb_mode (regs) ? " (T)" : "");
#if defined CONFIG_ARM_UNWIND && IN_PROPER
	unwind_backtrace(regs);
#endif
}

static void __noreturn do_exception(const char *reason, struct pt_regs *pt_regs)
{
	if (reason)
		eprintf("PANIC: unable to handle %s\n", reason);

	show_regs(pt_regs);

	panic_no_stacktrace("");
}

/**
 * The CPU runs into an undefined instruction. That really should not happen!
 * @param[in] pt_regs Register set content when the accident happens
 */
void do_undefined_instruction (struct pt_regs *pt_regs)
{
	do_exception("undefined instruction", pt_regs);
}

/**
 * The CPU catches a software interrupt
 * @param[in] pt_regs Register set content when the interrupt happens
 *
 * There is no function behind this feature. So what to do else than
 * a reset?
 */
void do_software_interrupt (struct pt_regs *pt_regs)
{
	do_exception("software interrupt", pt_regs);
}

/**
 * The CPU catches a prefetch abort. That really should not happen!
 * @param[in] pt_regs Register set content when the accident happens
 *
 * instruction fetch from an unmapped area
 */
void do_prefetch_abort (struct pt_regs *pt_regs)
{
	do_exception("prefetch abort", pt_regs);
}

static const char *data_abort_reason(ulong far)
{
	if (far < PAGE_SIZE)
		return "NULL pointer dereference";
	if (inside_stack_guard_page(far))
		return "stack overflow";

	return "paging request";
}

/**
 * The CPU catches a data abort. That really should not happen!
 * @param[in] pt_regs Register set content when the accident happens
 *
 * data fetch from an unmapped area
 */
void do_data_abort (struct pt_regs *pt_regs)
{
	u32 far;

	asm volatile ("mrc     p15, 0, %0, c6, c0, 0" : "=r" (far) : : "cc");

	eprintf("PANIC: unable to handle %s at address 0x%08x\n",
		data_abort_reason(far), far);

	do_exception(NULL, pt_regs);
}

/**
 * The CPU catches a fast interrupt request.
 * @param[in] pt_regs Register set content when the interrupt happens
 *
 * We never enable FIQs, so this should not happen
 */
void do_fiq (struct pt_regs *pt_regs)
{
	do_exception("fast interrupt request", pt_regs);
}

/**
 * The CPU catches a regular interrupt.
 * @param[in] pt_regs Register set content when the interrupt happens
 *
 * We never enable interrupts, so this should not happen
 */
void do_irq (struct pt_regs *pt_regs)
{
	do_exception("interrupt request", pt_regs);
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

static unsigned long arm_vbar = ~0;

unsigned long arm_get_vector_table(void)
{
	return arm_vbar;
}

#define ARM_HIGH_VECTORS	0xffff0000
#define ARM_LOW_VECTORS		0x0

/**
 * set_vector_table - let CPU use the vector table at given address
 * @adr - The address of the vector table
 *
 * Depending on the CPU the possibilities differ. ARMv7 and later allow
 * to map the vector table to arbitrary addresses. Other CPUs only allow
 * vectors at 0xffff0000 or at 0x0.
 */
static int set_vector_table(unsigned long adr)
{
	u32 cr;

	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		set_vbar(adr);
	} else if (adr == ARM_HIGH_VECTORS) {
		cr = get_cr();
		cr |= CR_V;
		set_cr(cr);
		cr = get_cr();
		if (!(cr & CR_V))
			return -EINVAL;
	} else if (adr == ARM_LOW_VECTORS) {
		cr = get_cr();
		cr &= ~CR_V;
		set_cr(cr);
		cr = get_cr();
		if (cr & CR_V)
			return -EINVAL;
	} else {
		return -EOPNOTSUPP;
	}

	pr_debug("Vectors are at 0x%08lx\n", adr);
	arm_vbar = adr;

	return 0;
}

static __maybe_unused int arm_init_vectors(void)
{
	/*
	 * First try to use the vectors where they actually are, works
	 * on ARMv7 and later.
	 */
	if (!set_vector_table((unsigned long)__exceptions_start))
		return 0;

	/*
	 * Next try high vectors at 0xffff0000.
	 */
	if (!set_vector_table(ARM_HIGH_VECTORS)) {
		create_vector_table(ARM_HIGH_VECTORS);
		return 0;
	}

	/*
	 * As a last resort use low vectors at 0x0. With this we can't
	 * set the zero page to faulting and can't catch NULL pointer
	 * exceptions.
	 */
	set_vector_table(ARM_LOW_VECTORS);
	create_vector_table(ARM_LOW_VECTORS);

	return 0;
}
#ifdef CONFIG_MMU
core_initcall(arm_init_vectors);
#endif

#if IS_ENABLED(CONFIG_ARM_EXCEPTIONS_PBL)
void arm_pbl_init_exceptions(void)
{
	if (cpu_architecture() < CPU_ARCH_ARMv7)
		return;

	set_vbar((unsigned long)__exceptions_start);
}
#endif

// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2018 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* interrupts_64.c - Interrupt Support Routines */

#include <common.h>
#include <abort.h>
#include <asm/ptrace.h>
#include <asm/barebox-arm.h>
#include <asm/unwind.h>
#include <init.h>
#include <asm/system.h>
#include <asm/proc-armv/system.h>
#include <asm/sysreg.h>
#include <asm/esr.h>
#include <efi/mode.h>

/* Avoid missing prototype warning, called from assembly */
void do_sync_sp_el0 (struct pt_regs *pt_regs);
void do_irq_sp_el0 (struct pt_regs *pt_regs);
void do_fiq_sp_el0 (struct pt_regs *pt_regs);
void do_serror_sp_el0 (struct pt_regs *pt_regs);
void do_fiq_sp_elx (struct pt_regs *pt_regs);
void do_irq_sp_elx (struct pt_regs *pt_regs);
void do_serror_sp_elx (struct pt_regs *pt_regs);
void do_sync_sp_elx(struct pt_regs *pt_regs, unsigned int esr, unsigned long far);

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

	eprintf("elr: %016lx lr : %016lx\n", regs->elr, regs->regs[30]);

	for (i = 0; i < 29; i += 2)
		eprintf("x%-2d: %016lx x%-2d: %016lx\n",
			i, regs->regs[i], i + 1, regs->regs[i + 1]);
	eprintf("\n");
}

static void __noreturn do_exception(const char *reason, struct pt_regs *pt_regs)
{
	if (reason)
		eprintf("PANIC: unable to handle %s\n", reason);

	show_regs(pt_regs);

	if (IN_PROPER)
		unwind_backtrace(pt_regs);

	panic_no_stacktrace("");
}

/**
 * The CPU catches a fast interrupt request.
 * @param[in] pt_regs Register set content when the interrupt happens
 *
 * We never enable FIQs, so this should not happen
 */
void do_fiq_sp_elx(struct pt_regs *pt_regs)
{
	do_exception("fiq request (SP_ELx)", pt_regs);
}

/**
 * The CPU catches a regular interrupt.
 * @param[in] pt_regs Register set content when the interrupt happens
 *
 * We never enable interrupts, so this should not happen
 */
void do_irq_sp_elx(struct pt_regs *pt_regs)
{
	do_exception("irq request (SP_ELx)", pt_regs);
}

void do_sync_sp_el0(struct pt_regs *pt_regs)
{
	do_exception("sync exception (SP_EL0)", pt_regs);
}

void do_irq_sp_el0(struct pt_regs *pt_regs)
{
	do_exception("irq request (SP_EL0)", pt_regs);
}

void do_fiq_sp_el0(struct pt_regs *pt_regs)
{
	do_exception("fiq request (SP_EL0)", pt_regs);
}

void do_serror_sp_el0(struct pt_regs *pt_regs)
{
	do_exception("serror exception (SP_EL0)", pt_regs);
}

extern volatile int arm_ignore_data_abort;
extern volatile int arm_data_abort_occurred;
extern volatile int arm_ignore_serror_exception;
extern volatile int arm_serror_exception_occurred;

static const char *data_abort_reason(ulong far)
{
	if (far < PAGE_SIZE)
		return "NULL pointer dereference";

	if (inside_stack_guard_page(far))
		return "stack overflow";

	return NULL;
}

void do_sync_sp_elx(struct pt_regs *pt_regs, unsigned int esr, unsigned long far)
{
	const char *extra = NULL;

	if ((esr >> ESR_ELx_EC_SHIFT) == ESR_ELx_EC_DABT_CUR) {
		if (arm_ignore_data_abort) {
			arm_data_abort_occurred = 1;
			pt_regs->elr += 4;
			return;
		}

		extra = data_abort_reason(far);
	}

	if (!extra)
		extra = "paging request - sync exception (SP_ELx)";

	eprintf("PANIC: unable to handle %s at address 0x%016lx\n",
		extra, far);

	eprintf("%s (ESR 0x%08x)\n", esr_get_class_string(esr), esr);
	do_exception(NULL, pt_regs);
}

void do_serror_sp_elx(struct pt_regs *pt_regs)
{
	if (arm_ignore_serror_exception) {
		arm_serror_exception_occurred = 1;
		return;
	}

	do_exception("serror exception (SP_ELx)", pt_regs);
}

static void set_vbar_for_current_el(unsigned long *vectors)
{
	switch (current_el()) {
	case 3:
		write_sysreg(vectors, vbar_el3);
		break;
	case 2:
		write_sysreg(vectors, vbar_el2);
		break;
	default:
		write_sysreg(vectors, vbar_el1);
		break;
	}

	isb();
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

void async_abort_mask(void)
{
	arm_serror_exception_occurred = 0;
	arm_ignore_serror_exception = 1;
}

int async_abort_unmask(void)
{
	dsb();
	isb();

	arm_ignore_serror_exception = 0;

	return arm_serror_exception_occurred != 0;
}

void set_exception_level_routing(bool enable)
{
	switch (current_el()) {
	case 3:
		/* Route all external Aborts and SError exceptions to EL3. */
		if (enable)
			sysreg_clear_set(scr_el3, 0, SCR_EL3_EA);
		else
			sysreg_clear_set(scr_el3, SCR_EL3_EA, 0);
		break;
	case 2:
		/* Route all exceptions from EL1 to EL2. */
		if (enable)
			sysreg_clear_set(hcr_el2, 0, HCR_EL2_TGE);
		else
			sysreg_clear_set(hcr_el2, HCR_EL2_TGE, 0);
		break;
	}
}

extern unsigned long vectors;

static int aarch64_init_vectors(void)
{
	if (efi_is_payload())
		return 0;

	set_vbar_for_current_el(&vectors);
	set_exception_level_routing(true);
	local_async_abort_enable();

	return 0;
}
core_initcall(aarch64_init_vectors);

#if IS_ENABLED(CONFIG_ARM_EXCEPTIONS_PBL)
void arm_pbl_init_exceptions(void)
{
	aarch64_init_vectors();
}
#endif

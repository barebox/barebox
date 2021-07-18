/* SPDX-License-Identifier: GPL-2.0 */
#ifndef RISCV_ASM_IRQ_H__
#define RISCV_ASM_IRQ_H__

#include <asm/csr.h>
#include <asm/system.h>
#include <asm/asm.h>
#include <asm/asm-offsets.h>
#include <asm/ptrace.h>

#ifndef __ASSEMBLY__
#include <asm/barebox-riscv.h>
void strap_entry(void);
void mtrap_entry(void);
unsigned long handle_trap(struct pt_regs *regs);

static inline void irq_init_vector(enum riscv_mode mode)
{
	switch (mode) {
#ifdef CONFIG_RISCV_EXCEPTIONS
	case RISCV_S_MODE:
		asm volatile ("csrw stvec, %0; csrw sie, zero" : :
			      "r"(strap_entry + get_runtime_offset()));
		break;
	case RISCV_M_MODE:
		asm volatile ("csrw mtvec, %0; csrw mie, zero" : :
			      "r"(mtrap_entry + get_runtime_offset()));
		break;
#endif
	default:
		break;
	}
}

#else

.macro	pt_regs_push ptr
	REG_S ra,  PT_RA(\ptr)	/* x1 */
	REG_S sp,  PT_SP(\ptr)	/* x2 */
	REG_S gp,  PT_GP(\ptr)	/* x3 */
	REG_S tp,  PT_TP(\ptr)	/* x4 */
	REG_S t0,  PT_T0(\ptr)	/* x5 */
	REG_S t1,  PT_T1(\ptr)	/* x6 */
	REG_S t2,  PT_T2(\ptr)	/* x7 */
	REG_S s0,  PT_S0(\ptr)	/* x8/fp */
	REG_S s1,  PT_S1(\ptr)	/* x9 */
	REG_S a0,  PT_A0(\ptr)	/* x10 */
	REG_S a1,  PT_A1(\ptr)	/* x11 */
	REG_S a2,  PT_A2(\ptr)	/* x12 */
	REG_S a3,  PT_A3(\ptr)	/* x13 */
	REG_S a4,  PT_A4(\ptr)	/* x14 */
	REG_S a5,  PT_A5(\ptr)	/* x15 */
	REG_S a6,  PT_A6(\ptr)	/* x16 */
	REG_S a7,  PT_A7(\ptr)	/* x17 */
	REG_S s2,  PT_S2(\ptr)	/* x18 */
	REG_S s3,  PT_S3(\ptr)	/* x19 */
	REG_S s4,  PT_S4(\ptr)	/* x20 */
	REG_S s5,  PT_S5(\ptr)	/* x21 */
	REG_S s6,  PT_S6(\ptr)	/* x22 */
	REG_S s7,  PT_S7(\ptr)	/* x23 */
	REG_S s8,  PT_S8(\ptr)	/* x24 */
	REG_S s9,  PT_S9(\ptr)	/* x25 */
	REG_S s10, PT_S10(\ptr)	/* x26 */
	REG_S s11, PT_S11(\ptr)	/* x27 */
	REG_S t3,  PT_T3(\ptr)	/* x28 */
	REG_S t4,  PT_T4(\ptr)	/* x29 */
	REG_S t5,  PT_T5(\ptr)	/* x30 */
	REG_S t6,  PT_T6(\ptr)	/* x31 */
.endm

.macro	pt_regs_pop ptr
	REG_L ra,  PT_RA(\ptr)	/* x1 */
	REG_L sp,  PT_SP(\ptr)	/* x2 */
	REG_L gp,  PT_GP(\ptr)	/* x3 */
	REG_L tp,  PT_TP(\ptr)	/* x4 */
	REG_L t0,  PT_T0(\ptr)	/* x5 */
	REG_L t1,  PT_T1(\ptr)	/* x6 */
	REG_L t2,  PT_T2(\ptr)	/* x7 */
	REG_L s0,  PT_S0(\ptr)	/* x8/fp */
	REG_L s1,  PT_S1(\ptr)	/* x9 */
	REG_L a0,  PT_A0(\ptr)	/* x10 */
	REG_L a1,  PT_A1(\ptr)	/* x11 */
	REG_L a2,  PT_A2(\ptr)	/* x12 */
	REG_L a3,  PT_A3(\ptr)	/* x13 */
	REG_L a4,  PT_A4(\ptr)	/* x14 */
	REG_L a5,  PT_A5(\ptr)	/* x15 */
	REG_L a6,  PT_A6(\ptr)	/* x16 */
	REG_L a7,  PT_A7(\ptr)	/* x17 */
	REG_L s2,  PT_S2(\ptr)	/* x18 */
	REG_L s3,  PT_S3(\ptr)	/* x19 */
	REG_L s4,  PT_S4(\ptr)	/* x20 */
	REG_L s5,  PT_S5(\ptr)	/* x21 */
	REG_L s6,  PT_S6(\ptr)	/* x22 */
	REG_L s7,  PT_S7(\ptr)	/* x23 */
	REG_L s8,  PT_S8(\ptr)	/* x24 */
	REG_L s9,  PT_S9(\ptr)	/* x25 */
	REG_L s10, PT_S10(\ptr)	/* x26 */
	REG_L s11, PT_S11(\ptr)	/* x27 */
	REG_L t3,  PT_T3(\ptr)	/* x28 */
	REG_L t4,  PT_T4(\ptr)	/* x29 */
	REG_L t5,  PT_T5(\ptr)	/* x30 */
	REG_L t6,  PT_T6(\ptr)	/* x31 */
.endm

#endif

#endif

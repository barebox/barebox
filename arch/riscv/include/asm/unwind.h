/* SPDX-License-Identifier: GPL-2.0 */
#ifndef RISCV_ASM_UNWIND_H__
#define RISCV_ASM_UNWIND_H__

struct pt_regs;

#if defined CONFIG_RISCV_UNWIND && !defined __PBL__
void unwind_backtrace(const struct pt_regs *regs);
#else
static inline void unwind_backtrace(const struct pt_regs *regs)
{
}
#endif

#endif

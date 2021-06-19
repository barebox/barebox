/* SPDX-License-Identifier: GPL-2.0 */
#ifndef RISCV_ASM_UNWIND_H__
#define RISCV_ASM_UNWIND_H__

struct pt_regs;

void unwind_backtrace(struct pt_regs *regs);

#endif

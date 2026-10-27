/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: Copyright (c) 2021 Ahmad Fatoum, Pengutronix */
#ifndef ASM_RISCV_COMMON_H
#define ASM_RISCV_COMMON_H

#include <linux/compiler.h>

/*
 * Taking the address of a label is not guaranteed to yield the runtime
 * address: clang materializes it through the GOT when compiling with
 * -fpic, which still holds the link-time address before relocation.
 * Use auipc, which is PC-relative by definition.
 */
static __always_inline unsigned long get_pc(void)
{
	unsigned long pc;

	__asm__ __volatile__("auipc %0, 0" : "=r"(pc));

	return pc;
}

#endif /* ASM_RISCV_COMMON_H */

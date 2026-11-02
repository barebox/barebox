/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: Copyright (c) 2021 Ahmad Fatoum, Pengutronix */
#ifndef ASM_RISCV_COMMON_H
#define ASM_RISCV_COMMON_H

#include <linux/compiler.h>

static __always_inline unsigned long get_pc(void)
{
	unsigned long pc;

	__asm__ __volatile__("auipc %0, 0" : "=r"(pc));

	return pc;
}

#endif /* ASM_RISCV_COMMON_H */

/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * Define the machine-dependent type `jmp_buf'.  MIPS version.
 * Copyright (C) 1992-2021 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 */

#ifndef _MIPS_BITS_SETJMP_H
#define _MIPS_BITS_SETJMP_H 1

#include <asm/sgidefs.h>

typedef struct __jmp_buf_internal_tag {
	/* Program counter.  */
	void *__pc;

	/* Stack pointer.  */
	void *__sp;

	/* Callee-saved registers s0 through s7.  */
	unsigned long __regs[8];

	/* The frame pointer.  */
	void *__fp;
} jmp_buf[1];

int setjmp(jmp_buf jmp) __attribute__((returns_twice));
void longjmp(jmp_buf jmp, int ret) __attribute__((noreturn));
int initjmp(jmp_buf jmp, void __attribute__((noreturn)) (*func)(void), void *stack_top);

#endif /* _MIPS_BITS_SETJMP_H */

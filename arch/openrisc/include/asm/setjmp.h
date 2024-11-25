/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Define the machine-dependent type `jmp_buf'.  OpenRISC version.
 * Copyright (C) 2021 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 */

#ifndef _OR1K_BITS_SETJMP_H
#define _OR1K_BITS_SETJMP_H  1

typedef long int jmp_buf[13];

int setjmp(jmp_buf jmp) __attribute__((returns_twice));
void longjmp(jmp_buf jmp, int ret) __attribute__((noreturn));
int initjmp(jmp_buf jmp, void __attribute__((noreturn)) (*func)(void), void *stack_top);

#include <asm-generic/setjmp.h>

#endif /* _OR1K_BITS_SETJMP_H */

/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2021 Jules Maselbas <jmaselbas@kalray.eu>, Kalray Inc. */

#ifndef _ASM_KVX_SETJMP_H_
#define _ASM_KVX_SETJMP_H_

#include <linux/types.h>

typedef u64 jmp_buf[22];

int initjmp(jmp_buf jmp, void __attribute__((noreturn)) (*func)(void), void *stack_top);
int setjmp(jmp_buf jmp) __attribute__((returns_twice));
void longjmp(jmp_buf jmp, int ret) __attribute__((noreturn));

#include <asm-generic/setjmp.h>

#endif /* _ASM_KVX_SETJMP_H_ */

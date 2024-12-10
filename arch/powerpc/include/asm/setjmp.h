// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2017 Theobroma Systems Design und Consulting GmbH
 * (C) Copyright 2016 Alexander Graf <agraf@suse.de>
 */

#ifndef _SETJMP_H_
#define _SETJMP_H_	1

#include <asm/types.h>

typedef struct __jmp_buf_internal_tag {
	long int __regs[24];
} jmp_buf[1];

int setjmp(jmp_buf jmp) __attribute__((returns_twice));
void longjmp(jmp_buf jmp, int ret) __attribute__((noreturn));

int initjmp(jmp_buf jmp, void __attribute__((noreturn)) (*func)(void), void *stack_top);

#include <asm-generic/setjmp.h>

#endif /* _SETJMP_H_ */

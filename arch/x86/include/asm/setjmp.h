/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Written by H. Peter Anvin <hpa@zytor.com>
 * Brought in from Linux v4.4 and modified for U-Boot
 * From Linux arch/um/sys-i386/setjmp.S
 */

#ifndef __setjmp_h
#define __setjmp_h

#include <linux/compiler.h>

struct jmp_buf_data {
#if defined CONFIG_X86_64
#define __sjlj_attr
	unsigned long __rip;
	unsigned long __rsp;
	unsigned long __rbp;
	unsigned long __rbx;
	unsigned long __r12;
	unsigned long __r13;
	unsigned long __r14;
	unsigned long __r15;
#elif defined CONFIG_X86_32
#define __sjlj_attr	__attribute__((regparm(3)))
	unsigned int __ebx;
	unsigned int __esp;
	unsigned int __ebp;
	unsigned int __esi;
	unsigned int __edi;
	unsigned int __eip;
#else
#error "Unsupported configuration"
#endif
};

typedef struct jmp_buf_data jmp_buf[1];

int setjmp(jmp_buf jmp) __attribute__((returns_twice)) __sjlj_attr;
void longjmp(jmp_buf jmp, int ret) __attribute__((noreturn)) __sjlj_attr;

int initjmp(jmp_buf jmp, void __attribute__((noreturn)) (*func)(void), void *stack_top) __sjlj_attr;

#include <asm-generic/setjmp.h>

#endif

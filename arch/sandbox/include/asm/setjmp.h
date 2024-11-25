/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SETJMP_H_
#define __SETJMP_H_

struct jmp_buf_data {
	unsigned char opaque[512] __attribute__((__aligned__(16)));
};

typedef struct jmp_buf_data jmp_buf[1];

int setjmp(jmp_buf jmp) __attribute__((returns_twice));
void longjmp(jmp_buf jmp, int ret) __attribute__((noreturn));

int initjmp(jmp_buf jmp, void __attribute__((noreturn)) (*func)(void), void *stack_top);

#endif

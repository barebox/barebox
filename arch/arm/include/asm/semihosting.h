/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_SEMIHOSTING_H
#define __ASM_ARM_SEMIHOSTING_H

#include <asm-generic/semihosting.h>
#include <asm/unified.h>

static inline void semihosting_putc(void *unused, int c)
{
	/* We may not be relocated yet here, so we push
	 * to stack and take address of that
	 */
#ifdef CONFIG_CPU_64
	asm volatile (
		"stp %0, xzr, [sp, #-16]!\n"
		"mov x1, sp\n"
		"mov x0, #0x03\n"
	 	"hlt #0xf000\n"
		"add sp, sp, #16\n"
		: /* No outputs */
		: "r" ((long)c)
		: "x0", "x1", "x2", "x3", "x12", "memory"
	);
#else
	asm volatile (
		"push {%0}\n"
		"mov r1, sp\n"
		"mov r0, #0x03\n"
	 ARM(	"bkpt #0x123456\n")
	 THUMB(	"bkpt #0xAB\n")
		"pop {%0}\n"
		: /* No outputs */
		: "r" (c)
		: "r0", "r1", "r2", "r3", "r12", "memory"
	);
#endif
}

#endif

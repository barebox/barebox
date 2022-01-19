/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_COMMON_H
#define __ASM_ARM_COMMON_H

#include <linux/compiler.h>

static inline unsigned long get_pc(void)
{
	unsigned long pc;
#ifdef CONFIG_CPU_32
	__asm__ __volatile__(
                "mov    %0, pc\n"
                : "=r" (pc)
                :
                : "memory");
#else
	__asm__ __volatile__(
                "adr    %0, .\n"
                : "=r" (pc)
                :
                : "memory");
#endif
	return pc;
}

static inline unsigned long get_lr(void)
{
	unsigned long lr;

	__asm__ __volatile__(
                "mov    %0, lr\n"
                : "=r" (lr)
                :
                : "memory");

	return lr;
}

static inline unsigned long get_sp(void)
{
	unsigned long sp;

	__asm__ __volatile__(
                "mov    %0, sp\n"
                : "=r" (sp)
                :
                : "memory");

	return sp;
}

extern void __compiletime_error(
	"arm_setup_stack() called outside of naked function. On ARM64, "
	"stack should be setup in non-inline assembly before branching to C entry point."
) __unsafe_setup_stack(void);

/*
 * Sets up new stack growing down from top within a naked C function.
 * The first stack word will be top - sizeof(word).
 *
 * Avoid interleaving with C code as much as possible and jump
 * ASAP to a noinline function.
 */
static inline void arm_setup_stack(unsigned long top)
{
	if (IS_ENABLED(CONFIG_CPU_64))
		__unsafe_setup_stack();

	__asm__ __volatile__("mov sp, %0"
			     :
			     : "r"(top));
}

#endif /* __ASM_ARM_COMMON_H */

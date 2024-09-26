/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2008 ARM Limited */

/* Unified Assembler Syntax helper macros */

#ifndef __ASM_UNIFIED_H
#define __ASM_UNIFIED_H

#if defined(__ASSEMBLY__) && defined(CONFIG_CPU_32)
	.syntax unified
#endif

#ifdef CONFIG_THUMB2_BAREBOX

#if __GNUC__ < 4
#error Thumb-2 barebox requires gcc >= 4
#endif

/* The CPSR bit describing the instruction set (Thumb) */
#define PSR_ISETSTATE	PSR_T_BIT

#define ARM(x...)
#define THUMB(x...)	x
#ifdef __ASSEMBLY__
#define W(instr)	instr.w
#endif
#define BSYM(sym)	sym + 1

#else	/* !CONFIG_THUMB2_BAREBOX */

/* The CPSR bit describing the instruction set (ARM) */
#define PSR_ISETSTATE	0

#define ARM(x...)	x
#define THUMB(x...)
#ifdef __ASSEMBLY__
#define W(instr)	instr
#endif
#define BSYM(sym)	sym

#endif	/* CONFIG_THUMB2_BAREBOX */

#endif	/* !__ASM_UNIFIED_H */

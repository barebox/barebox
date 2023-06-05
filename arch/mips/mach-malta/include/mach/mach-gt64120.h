/* SPDX-License-Identifier: GPL-2.0-only */

/*
 *  This is a direct copy of the ev96100.h file, with a global
 * search and replace.	The numbers are the same.
 *
 *  The reason I'm duplicating this is so that the 64120/96100
 * defines won't be confusing in the source code.
 */
#ifndef _ASM_MACH_MIPS_MACH_GT64120_DEP_H
#define _ASM_MACH_MIPS_MACH_GT64120_DEP_H

#define MIPS_GT_BASE	0x1be00000

#define GT64120_BASE    CKSEG1ADDR(MIPS_GT_BASE)

#endif /* _ASM_MACH_MIPS_MACH_GT64120_DEP_H */

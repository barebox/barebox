/* SPDX-License-Identifier: GPL-2.0-or-later */

/**
 * @file
 * @brief mips specific string optimizations
 *
 * Thanks to the Linux kernel here we can add many micro optimized string
 * functions. But currently it makes no sense, to do so.
 */
#ifndef __ASM_MIPS_STRING_H
#define __ASM_MIPS_STRING_H

#ifdef CONFIG_MIPS_OPTIMIZED_STRING_FUNCTIONS

#define __HAVE_ARCH_MEMCPY
extern void *memcpy(void *, const void *, __kernel_size_t);
#define __HAVE_ARCH_MEMSET
extern void *memset(void *, int, __kernel_size_t);

#endif

#endif

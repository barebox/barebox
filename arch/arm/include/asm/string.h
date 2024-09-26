/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_STRING_H
#define __ASM_ARM_STRING_H

#ifdef CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS

#define __HAVE_ARCH_MEMCPY
extern void *memcpy(void *, const void *, __kernel_size_t);
#define __HAVE_ARCH_MEMSET
extern void *memset(void *, int, __kernel_size_t);
#define __HAVE_ARCH_MEMMOVE
extern void *memmove(void *, const void *, __kernel_size_t);
#endif

extern void *__memcpy(void *, const void *, __kernel_size_t);
extern void *__memset(void *, int, __kernel_size_t);
extern void *__memmove(void *, const void *, __kernel_size_t);

#endif

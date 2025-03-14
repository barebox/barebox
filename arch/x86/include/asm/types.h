/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_I386_TYPES_H
#define __ASM_I386_TYPES_H

#include <asm-generic/int-ll64.h>

#ifndef __ASSEMBLY__

#ifdef __x86_64__
/*
 * This is used in dlmalloc. On X86_64 we need it to be
 * 64 bit
 */
#define INTERNAL_SIZE_T unsigned long

#endif

#endif

#endif

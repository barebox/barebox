/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generic non-atomic cmpxchg for porting kernel code.
 */

#ifndef __ASM_GENERIC_CMPXCHG_H
#define __ASM_GENERIC_CMPXCHG_H

#ifdef CONFIG_SMP
#error "Cannot use generic cmpxchg on SMP"
#endif

#include <asm-generic/cmpxchg-local.h>

#define generic_cmpxchg_local(ptr, o, n) ({					\
	((__typeof__(*(ptr)))__generic_cmpxchg_local((ptr), (unsigned long)(o),	\
			(unsigned long)(n), sizeof(*(ptr))));			\
})

#define generic_cmpxchg64_local(ptr, o, n) \
	__generic_cmpxchg64_local((ptr), (o), (n))

#define cmpxchg		generic_cmpxchg_local
#define cmpxchg64	generic_cmpxchg64_local

#endif

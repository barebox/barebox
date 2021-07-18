/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 Regents of the University of California
 */

#ifndef _ASM_RISCV_CACHEFLUSH_H
#define _ASM_RISCV_CACHEFLUSH_H

static inline void local_flush_icache_all(void)
{
#ifdef HAS_CACHE
	asm volatile ("fence.i" ::: "memory");
#endif
}

#endif /* _ASM_RISCV_CACHEFLUSH_H */

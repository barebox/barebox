/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 Regents of the University of California
 */

#ifndef _ASM_RISCV_CACHE_H
#define _ASM_RISCV_CACHE_H

#include <asm/vendorid_list.h>

static inline void thead_local_flush_icache_all(void)
{
	/*
	 * According [1] "13.3 Example of cache settings"
	 * [1]: https://github.com/T-head-Semi/openc906/blob/main/ \
	 *	doc/openc906%20datasheet.pd
	 */
	__asm__ volatile (".long 0x0100000b" ::: "memory"); /* th.icache.iall */
	__asm__ volatile (".long 0x01b0000b" ::: "memory"); /* th.sync.is */
}

static inline void local_flush_icache_all(void)
{
#ifdef CONFIG_RISCV_ICACHE
	switch(riscv_vendor_id()) {
	case THEAD_VENDOR_ID:
		thead_local_flush_icache_all();
		break;
	default:
		__asm__ volatile ("fence.i" ::: "memory");
	}
#endif
}

#define sync_caches_for_execution sync_caches_for_execution
void sync_caches_for_execution(void);

#include <asm-generic/cache.h>

#endif /* _ASM_RISCV_CACHEFLUSH_H */

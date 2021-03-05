/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef __KVX_CACHE_H
#define __KVX_CACHE_H

#include <linux/types.h>

void invalidate_dcache_range(unsigned long addr, unsigned long stop);

static inline void sync_caches_for_execution(void)
{
	__builtin_kvx_fence();
	__builtin_kvx_iinval();
	__builtin_kvx_barrier();
}

static inline void sync_dcache_icache(void)
{
	sync_caches_for_execution();
}

static inline void dcache_inval(void)
{
	__builtin_kvx_fence();
	__builtin_kvx_dinval();
}

#endif /* __KVX_CACHE_H */

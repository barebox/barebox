// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2021 Yann Sionneau <ysionneau@kalray.eu>, Kalray Inc.

#include <asm/cache.h>
#include <linux/kernel.h>

#define KVX_DCACHE_LINE_SIZE    (64)
#define KVX_DCACHE_SIZE (128*1024)

void invalidate_dcache_range(unsigned long addr, unsigned long stop)
{
	long size;

	addr = ALIGN_DOWN(addr, KVX_DCACHE_LINE_SIZE);
	size = stop - addr;

	if (size < KVX_DCACHE_SIZE) {
		while (addr < stop) {
			__builtin_kvx_dinvall((void *)addr);
			addr += KVX_DCACHE_LINE_SIZE;
		}
	} else {
		__builtin_kvx_dinval();
	}
}

// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/system_info.h>

/*
 * Early function to flush the caches. This is for use when the
 * C environment is not yet fully initialized.
 */
void arm_early_mmu_cache_flush(void)
{
	v8_flush_dcache_all();
	v8_invalidate_icache_all();
}

void arm_early_mmu_cache_invalidate(void)
{
	v8_invalidate_dcache_all();
	v8_invalidate_icache_all();
}

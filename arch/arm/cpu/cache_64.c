/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/system_info.h>

int arm_set_cache_functions(void)
{
	return 0;
}

/*
 * Early function to flush the caches. This is for use when the
 * C environment is not yet fully initialized.
 */
void arm_early_mmu_cache_flush(void)
{
	v8_flush_dcache_all();
}

void arm_early_mmu_cache_invalidate(void)
{
	v8_invalidate_icache_all();
}

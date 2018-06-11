#include <common.h>
#include <asm/mmu.h>
#include <errno.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/system.h>
#include <asm/cache.h>

#include "mmu.h"

static uint32_t *ttb;

static void map_cachable(unsigned long start, unsigned long size)
{
	start = ALIGN_DOWN(start, SZ_1M);
	size  = ALIGN(size, SZ_1M);

	create_sections(ttb, start, start + size - 1, PMD_SECT_DEF_CACHED);
}

void mmu_early_enable(unsigned long membase, unsigned long memsize,
		      unsigned long _ttb)
{
	ttb = (uint32_t *)_ttb;

	arm_set_cache_functions();

	set_ttbr(ttb);
	set_domain(DOMAIN_MANAGER);

	create_flat_mapping(ttb);

	map_cachable(membase, memsize);

	__mmu_cache_on();
}

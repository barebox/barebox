#include <common.h>
#include <asm/mmu.h>
#include <errno.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <asm-generic/sections.h>

#include "mmu.h"

static uint32_t *ttb;

static inline void map_region(unsigned long start, unsigned long size,
			      uint64_t flags)

{
	start = ALIGN_DOWN(start, SZ_1M);
	size  = ALIGN(size, SZ_1M);

	create_sections(ttb, start, start + size - 1, flags);
}

void mmu_early_enable(unsigned long membase, unsigned long memsize,
		      unsigned long _ttb)
{
	ttb = (uint32_t *)_ttb;

	arm_set_cache_functions();

	set_ttbr(ttb);
	set_domain(DOMAIN_MANAGER);

	/*
	 * This marks the whole address space as uncachable as well as
	 * unexecutable if possible
	 */
	create_flat_mapping(ttb);

	/*
	 * There can be SoCs that have a section shared between device memory
	 * and the on-chip RAM hosting the PBL. Thus mark this section
	 * uncachable, but executable.
	 * On such SoCs, executing from OCRAM could cause the instruction
	 * prefetcher to speculatively access that device memory, triggering
	 * potential errant behavior.
	 *
	 * If your SoC has such a memory layout, you should rewrite the code
	 * here to map the OCRAM page-wise.
	 */
	map_region((unsigned long)_stext, _etext - _stext, PMD_SECT_DEF_UNCACHED);

	/* maps main memory as cachable */
	map_region(membase, memsize, PMD_SECT_DEF_CACHED);

	__mmu_cache_on();
}

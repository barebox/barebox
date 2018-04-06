#include <common.h>
#include <dma-dir.h>
#include <init.h>
#include <mmu.h>
#include <errno.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/pgtable64.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <memory.h>
#include <asm/system_info.h>

#include "mmu_64.h"

static void create_sections(void *ttb, uint64_t virt, uint64_t phys,
			    uint64_t size, uint64_t attr)
{
	uint64_t block_size;
	uint64_t block_shift;
	uint64_t *pte;
	uint64_t idx;
	uint64_t addr;
	uint64_t *table;

	addr = virt;

	attr &= ~PTE_TYPE_MASK;

	table = ttb;

	while (1) {
		block_shift = level2shift(1);
		idx = (addr & level2mask(1)) >> block_shift;
		block_size = (1ULL << block_shift);

		pte = table + idx;

		*pte = phys | attr | PTE_TYPE_BLOCK;

		if (size < block_size)
			break;

		addr += block_size;
		phys += block_size;
		size -= block_size;
	}
}

void mmu_early_enable(unsigned long membase, unsigned long memsize,
		      unsigned long ttb)
{
	int el;

	/*
	 * For the early code we only create level 1 pagetables which only
	 * allow for a 1GiB granularity. If our membase is not aligned to that
	 * bail out without enabling the MMU.
	 */
	if (membase & ((1ULL << level2shift(1)) - 1))
		return;

	memset((void *)ttb, 0, GRANULE_SIZE);

	el = current_el();
	set_ttbr_tcr_mair(el, ttb, calc_tcr(el), MEMORY_ATTRIBUTES);
	create_sections((void *)ttb, 0, 0, 1UL << (BITS_PER_VA - 1), UNCACHED_MEM);
	create_sections((void *)ttb, membase, membase, memsize, CACHED_MEM);
	tlb_invalidate();
	isb();
	set_cr(get_cr() | CR_M);
}

void mmu_early_disable(void)
{
	unsigned int cr;

	cr = get_cr();
	cr &= ~(CR_M | CR_C);

	set_cr(cr);
	v8_flush_dcache_all();
	tlb_invalidate();

	dsb();
	isb();
}
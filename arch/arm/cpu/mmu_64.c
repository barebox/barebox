// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2009-2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2016 Raphaël Poggi <poggi.raph@gmail.com>

#define pr_fmt(fmt)	"mmu: " fmt

#include <common.h>
#include <dma.h>
#include <dma-dir.h>
#include <init.h>
#include <mmu.h>
#include <errno.h>
#include <zero_page.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/pgtable64.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <memory.h>
#include <asm/system_info.h>

#include "mmu_64.h"

static uint64_t *ttb;

static void set_table(uint64_t *pt, uint64_t *table_addr)
{
	uint64_t val;

	val = PTE_TYPE_TABLE | (uint64_t)table_addr;
	*pt = val;
}

#ifdef __PBL__
static uint64_t *alloc_pte(void)
{
	static unsigned int idx;

	idx++;

	if (idx * GRANULE_SIZE >= ARM_EARLY_PAGETABLE_SIZE)
		return NULL;

	return (void *)ttb + idx * GRANULE_SIZE;
}
#else
static uint64_t *alloc_pte(void)
{
	uint64_t *new_table = xmemalign(GRANULE_SIZE, GRANULE_SIZE);

	/* Mark all entries as invalid */
	memset(new_table, 0, GRANULE_SIZE);

	return new_table;
}
#endif

static __maybe_unused uint64_t *find_pte(uint64_t addr)
{
	uint64_t *pte;
	uint64_t block_shift;
	uint64_t idx;
	int i;

	pte = ttb;

	for (i = 0; i < 4; i++) {
		block_shift = level2shift(i);
		idx = (addr & level2mask(i)) >> block_shift;
		pte += idx;

		if ((pte_type(pte) != PTE_TYPE_TABLE) || (block_shift <= GRANULE_SIZE_SHIFT))
			break;
		else
			pte = (uint64_t *)(*pte & XLAT_ADDR_MASK);
	}

	return pte;
}

#define MAX_PTE_ENTRIES 512

/* Splits a block PTE into table with subpages spanning the old block */
static void split_block(uint64_t *pte, int level)
{
	uint64_t old_pte = *pte;
	uint64_t *new_table;
	uint64_t i = 0;
	int levelshift;

	if ((*pte & PTE_TYPE_MASK) == PTE_TYPE_TABLE)
		return;

	/* level describes the parent level, we need the child ones */
	levelshift = level2shift(level + 1);

	new_table = alloc_pte();

	for (i = 0; i < MAX_PTE_ENTRIES; i++) {
		new_table[i] = old_pte | (i << levelshift);

		/* Level 3 block PTEs have the table type */
		if ((level + 1) == 3)
			new_table[i] |= PTE_TYPE_TABLE;
	}

	/* Set the new table into effect */
	set_table(pte, new_table);
}

static void create_sections(uint64_t virt, uint64_t phys, uint64_t size,
			    uint64_t attr)
{
	uint64_t block_size;
	uint64_t block_shift;
	uint64_t *pte;
	uint64_t idx;
	uint64_t addr;
	uint64_t *table;
	uint64_t type;
	int level;

	if (!ttb)
		arm_mmu_not_initialized_error();

	addr = virt;

	attr &= ~PTE_TYPE_MASK;

	while (size) {
		table = ttb;
		for (level = 0; level < 4; level++) {
			block_shift = level2shift(level);
			idx = (addr & level2mask(level)) >> block_shift;
			block_size = (1ULL << block_shift);

			pte = table + idx;

			if (size >= block_size && IS_ALIGNED(addr, block_size)) {
				type = (level == 3) ?
					PTE_TYPE_PAGE : PTE_TYPE_BLOCK;
				*pte = phys | attr | type;
				addr += block_size;
				phys += block_size;
				size -= block_size;
				break;
			} else {
				split_block(pte, level);
			}

			table = get_level_table(pte);
		}

	}

	tlb_invalidate();
}

int arch_remap_range(void *_start, size_t size, unsigned flags)
{
	unsigned long attrs;

	switch (flags) {
	case MAP_CACHED:
		attrs = CACHED_MEM;
		break;
	case MAP_UNCACHED:
		attrs = attrs_uncached_mem();
		break;
	case MAP_FAULT:
		attrs = 0x0;
		break;
	default:
		return -EINVAL;
	}

	create_sections((uint64_t)_start, (uint64_t)_start, (uint64_t)size,
			attrs);
	return 0;
}

static void mmu_enable(void)
{
	isb();
	set_cr(get_cr() | CR_M | CR_C | CR_I);
}

/*
 * Prepare MMU for usage enable it.
 */
void __mmu_init(bool mmu_on)
{
	struct memory_bank *bank;
	unsigned int el;

	if (mmu_on)
		mmu_disable();

	ttb = alloc_pte();
	el = current_el();
	set_ttbr_tcr_mair(el, (uint64_t)ttb, calc_tcr(el, BITS_PER_VA),
			  MEMORY_ATTRIBUTES);

	pr_debug("ttb: 0x%p\n", ttb);

	/* create a flat mapping */
	arch_remap_range(0, 1UL << (BITS_PER_VA - 1), MAP_UNCACHED);

	/* Map sdram cached. */
	for_each_memory_bank(bank) {
		struct resource *rsv;

		arch_remap_range((void *)bank->start, bank->size, MAP_CACHED);

		for_each_reserved_region(bank, rsv) {
			arch_remap_range((void *)resource_first_page(rsv),
					 resource_count_pages(rsv), MAP_UNCACHED);
		}
	}

	/* Make zero page faulting to catch NULL pointer derefs */
	zero_page_faulting();

	mmu_enable();
}

void mmu_disable(void)
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

void dma_inv_range(void *ptr, size_t size)
{
	unsigned long start = (unsigned long)ptr;
	unsigned long end = start + size - 1;

	v8_inv_dcache_range(start, end);
}

void dma_flush_range(void *ptr, size_t size)
{
	unsigned long start = (unsigned long)ptr;
	unsigned long end = start + size - 1;

	v8_flush_dcache_range(start, end);
}

static void early_create_sections(void *ttb, uint64_t virt, uint64_t phys,
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

#define EARLY_BITS_PER_VA 39

void mmu_early_enable(unsigned long membase, unsigned long memsize)
{
	int el;
	unsigned long ttb = arm_mem_ttb(membase + memsize);

	pr_debug("enabling MMU, ttb @ 0x%08lx\n", ttb);

	/*
	 * For the early code we only create level 1 pagetables which only
	 * allow for a 1GiB granularity. If our membase is not aligned to that
	 * bail out without enabling the MMU.
	 */
	if (membase & ((1ULL << level2shift(1)) - 1))
		return;

	memset((void *)ttb, 0, GRANULE_SIZE);

	el = current_el();
	set_ttbr_tcr_mair(el, ttb, calc_tcr(el, EARLY_BITS_PER_VA), MEMORY_ATTRIBUTES);
	early_create_sections((void *)ttb, 0, 0, 1UL << (EARLY_BITS_PER_VA - 1),
			attrs_uncached_mem());
	early_create_sections((void *)ttb, membase, membase, memsize - OPTEE_SIZE, CACHED_MEM);
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

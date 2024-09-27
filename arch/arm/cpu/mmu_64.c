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
#include <linux/pagemap.h>
#include <tee/optee.h>

#include "mmu_64.h"

#define ARCH_MAP_WRITECOMBINE  ((unsigned)-1)

static uint64_t *get_ttb(void)
{
	return (uint64_t *)get_ttbr(current_el());
}

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

	return (void *)get_ttb() + idx * GRANULE_SIZE;
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

	pte = get_ttb();

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
	if (!new_table)
		panic("Unable to allocate PTE\n");


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
	uint64_t *ttb = get_ttb();
	uint64_t block_size;
	uint64_t block_shift;
	uint64_t *pte;
	uint64_t idx;
	uint64_t addr;
	uint64_t *table;
	uint64_t type;
	int level;

	addr = virt;

	attr &= ~PTE_TYPE_MASK;

	size = PAGE_ALIGN(size);

	while (size) {
		table = ttb;
		for (level = 0; level < 4; level++) {
			block_shift = level2shift(level);
			idx = (addr & level2mask(level)) >> block_shift;
			block_size = (1ULL << block_shift);

			pte = table + idx;

			if (size >= block_size && IS_ALIGNED(addr, block_size) &&
			    IS_ALIGNED(phys, block_size)) {
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

static unsigned long get_pte_attrs(unsigned flags)
{
	switch (flags) {
	case MAP_CACHED:
		return CACHED_MEM;
	case MAP_UNCACHED:
		return attrs_xn() | UNCACHED_MEM;
	case MAP_FAULT:
		return 0x0;
	case ARCH_MAP_WRITECOMBINE:
		return attrs_xn() | MEM_ALLOC_WRITECOMBINE;
	default:
		return ~0UL;
	}
}

static void early_remap_range(uint64_t addr, size_t size, unsigned flags)
{
	unsigned long attrs = get_pte_attrs(flags);

	if (WARN_ON(attrs == ~0UL))
		return;

	create_sections(addr, addr, size, attrs);
}

int arch_remap_range(void *virt_addr, phys_addr_t phys_addr, size_t size, unsigned flags)
{
	unsigned long attrs = get_pte_attrs(flags);

	if (attrs == ~0UL)
		return -EINVAL;

	create_sections((uint64_t)virt_addr, phys_addr, (uint64_t)size, attrs);

	if (flags == MAP_UNCACHED)
		dma_inv_range(virt_addr, size);

	return 0;
}

static void mmu_enable(void)
{
	isb();
	set_cr(get_cr() | CR_M | CR_C | CR_I);
}

static void create_guard_page(void)
{
	ulong guard_page;

	if (!IS_ENABLED(CONFIG_STACK_GUARD_PAGE))
		return;

	guard_page = arm_mem_guard_page_get();
	request_barebox_region("guard page", guard_page, PAGE_SIZE);
	remap_range((void *)guard_page, PAGE_SIZE, MAP_FAULT);

	pr_debug("Created guard page\n");
}

/*
 * Prepare MMU for usage enable it.
 */
void __mmu_init(bool mmu_on)
{
	uint64_t *ttb = get_ttb();
	struct memory_bank *bank;

	if (!request_barebox_region("ttb", (unsigned long)ttb,
				  ARM_EARLY_PAGETABLE_SIZE))
		/*
		 * This can mean that:
		 * - the early MMU code has put the ttb into a place
		 *   which we don't have inside our available memory
		 * - Somebody else has occupied the ttb region which means
		 *   the ttb will get corrupted.
		 */
		pr_crit("Can't request SDRAM region for ttb at %p\n", ttb);

	for_each_memory_bank(bank) {
		struct resource *rsv;
		resource_size_t pos;

		pos = bank->start;

		/* Skip reserved regions */
		for_each_reserved_region(bank, rsv) {
			remap_range((void *)pos, rsv->start - pos, MAP_CACHED);
			pos = rsv->end + 1;
		}

		remap_range((void *)pos, bank->start + bank->size - pos, MAP_CACHED);
	}

	/* Make zero page faulting to catch NULL pointer derefs */
	zero_page_faulting();
	create_guard_page();
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
	unsigned long end = start + size;

	v8_inv_dcache_range(start, end);
}

void dma_flush_range(void *ptr, size_t size)
{
	unsigned long start = (unsigned long)ptr;
	unsigned long end = start + size;

	v8_flush_dcache_range(start, end);
}

void *dma_alloc_writecombine(size_t size, dma_addr_t *dma_handle)
{
	return dma_alloc_map(size, dma_handle, ARCH_MAP_WRITECOMBINE);
}

static void init_range(size_t total_level0_tables)
{
	uint64_t *ttb = get_ttb();
	uint64_t addr = 0;

	while (total_level0_tables--) {
		early_remap_range(addr, L0_XLAT_SIZE, MAP_UNCACHED);
		split_block(ttb, 0);
		addr += L0_XLAT_SIZE;
		ttb++;
	}
}

void mmu_early_enable(unsigned long membase, unsigned long memsize)
{
	int el;
	u64 optee_membase;
	unsigned long ttb = arm_mem_ttb(membase + memsize);

	if (get_cr() & CR_M)
		return;

	pr_debug("enabling MMU, ttb @ 0x%08lx\n", ttb);

	el = current_el();
	set_ttbr_tcr_mair(el, ttb, calc_tcr(el, BITS_PER_VA), MEMORY_ATTRIBUTES);
	if (el == 3)
		set_ttbr_tcr_mair(2, ttb, calc_tcr(2, BITS_PER_VA), MEMORY_ATTRIBUTES);

	memset((void *)ttb, 0, GRANULE_SIZE);

	/*
	 * Assume maximum BITS_PER_PA set to 40 bits.
	 * Set 1:1 mapping of VA->PA. So to cover the full 1TB range we need 2 tables.
	 */
	init_range(2);

	early_remap_range(membase, memsize, MAP_CACHED);

	if (optee_get_membase(&optee_membase))
                optee_membase = membase + memsize - OPTEE_SIZE;

	early_remap_range(optee_membase, OPTEE_SIZE, MAP_FAULT);

	early_remap_range(PAGE_ALIGN_DOWN((uintptr_t)_stext), PAGE_ALIGN(_etext - _stext), MAP_CACHED);

	mmu_enable();
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

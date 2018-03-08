/*
 * Copyright (c) 2009-2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2016 Raphaël Poggi <poggi.raph@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt)	"mmu: " fmt

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

#include "mmu.h"

#define CACHED_MEM      (PTE_BLOCK_MEMTYPE(MT_NORMAL) | \
			 PTE_BLOCK_OUTER_SHARE | \
			 PTE_BLOCK_AF)
#define UNCACHED_MEM    (PTE_BLOCK_MEMTYPE(MT_DEVICE_nGnRnE) | \
			 PTE_BLOCK_OUTER_SHARE | \
			 PTE_BLOCK_AF)

static uint64_t *ttb;

static void arm_mmu_not_initialized_error(void)
{
	/*
	 * This means:
	 * - one of the MMU functions like dma_alloc_coherent
	 *   or remap_range is called too early, before the MMU is initialized
	 * - Or the MMU initialization has failed earlier
	 */
	panic("MMU not initialized\n");
}


/*
 * Do it the simple way for now and invalidate the entire
 * tlb
 */
static inline void tlb_invalidate(void)
{
	unsigned int el = current_el();

	dsb();

	if (el == 1)
		__asm__ __volatile__("tlbi vmalle1\n\t" : : : "memory");
	else if (el == 2)
		__asm__ __volatile__("tlbi alle2\n\t" : : : "memory");
	else if (el == 3)
		__asm__ __volatile__("tlbi alle3\n\t" : : : "memory");

	dsb();
	isb();
}

static int level2shift(int level)
{
	/* Page is 12 bits wide, every level translates 9 bits */
	return (12 + 9 * (3 - level));
}

static uint64_t level2mask(int level)
{
	uint64_t mask = -EINVAL;

	if (level == 1)
		mask = L1_ADDR_MASK;
	else if (level == 2)
		mask = L2_ADDR_MASK;
	else if (level == 3)
		mask = L3_ADDR_MASK;

	return mask;
}

static int pte_type(uint64_t *pte)
{
	return *pte & PTE_TYPE_MASK;
}

static void set_table(uint64_t *pt, uint64_t *table_addr)
{
	uint64_t val;

	val = PTE_TYPE_TABLE | (uint64_t)table_addr;
	*pt = val;
}

static uint64_t *create_table(void)
{
	uint64_t *new_table = xmemalign(GRANULE_SIZE, GRANULE_SIZE);

	/* Mark all entries as invalid */
	memset(new_table, 0, GRANULE_SIZE);

	return new_table;
}

static uint64_t *get_level_table(uint64_t *pte)
{
	uint64_t *table = (uint64_t *)(*pte & XLAT_ADDR_MASK);

	if (pte_type(pte) != PTE_TYPE_TABLE)
		BUG();

	return table;
}

static __maybe_unused uint64_t *find_pte(uint64_t addr)
{
	uint64_t *pte;
	uint64_t block_shift;
	uint64_t idx;
	int i;

	pte = ttb;

	for (i = 1; i < 4; i++) {
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

	new_table = create_table();

	for (i = 0; i < MAX_PTE_ENTRIES; i++) {
		new_table[i] = old_pte | (i << levelshift);

		/* Level 3 block PTEs have the table type */
		if ((level + 1) == 3)
			new_table[i] |= PTE_TYPE_TABLE;
	}

	/* Set the new table into effect */
	set_table(pte, new_table);
}

static void map_region(uint64_t virt, uint64_t phys, uint64_t size, uint64_t attr)
{
	uint64_t block_size;
	uint64_t block_shift;
	uint64_t *pte;
	uint64_t idx;
	uint64_t addr;
	uint64_t *table;
	int level;

	if (!ttb)
		arm_mmu_not_initialized_error();

	addr = virt;

	attr &= ~PTE_TYPE_MASK;

	while (size) {
		table = ttb;
		for (level = 1; level < 4; level++) {
			block_shift = level2shift(level);
			idx = (addr & level2mask(level)) >> block_shift;
			block_size = (1ULL << block_shift);

			pte = table + idx;

			if (level == 3)
				attr |= PTE_TYPE_PAGE;
			else
				attr |= PTE_TYPE_BLOCK;

			if (size >= block_size && IS_ALIGNED(addr, block_size)) {
				*pte = phys | attr;
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
}

static void create_sections(uint64_t virt, uint64_t phys, uint64_t size, uint64_t flags)
{
	map_region(virt, phys, size, flags);
	tlb_invalidate();
}

int arch_remap_range(void *_start, size_t size, unsigned flags)
{
	map_region((uint64_t)_start, (uint64_t)_start, (uint64_t)size, flags);
	tlb_invalidate();

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
static int mmu_init(void)
{
	struct memory_bank *bank;

	if (list_empty(&memory_banks))
		/*
		 * If you see this it means you have no memory registered.
		 * This can be done either with arm_add_mem_device() in an
		 * initcall prior to mmu_initcall or via devicetree in the
		 * memory node.
		 */
		panic("MMU: No memory bank found! Cannot continue\n");

	if (get_cr() & CR_M) {
		ttb = (uint64_t *)get_ttbr(current_el());
		if (!request_sdram_region("ttb", (unsigned long)ttb, SZ_16K))
			/*
			* This can mean that:
			* - the early MMU code has put the ttb into a place
			*   which we don't have inside our available memory
			* - Somebody else has occupied the ttb region which means
			*   the ttb will get corrupted.
			*/
			pr_crit("Critical Error: Can't request SDRAM region for ttb at %p\n",
				ttb);
	} else {
		ttb = xmemalign(GRANULE_SIZE, GRANULE_SIZE);

		memset(ttb, 0, GRANULE_SIZE);

		set_ttbr_tcr_mair(current_el(), (uint64_t)ttb, TCR_FLAGS,
				  MEMORY_ATTRIBUTES);
	}

	pr_debug("ttb: 0x%p\n", ttb);

	/* create a flat mapping */
	create_sections(0, 0, 1UL << (BITS_PER_VA - 1), UNCACHED_MEM);

	/* Map sdram cached. */
	for_each_memory_bank(bank)
		create_sections(bank->start, bank->start, bank->size, CACHED_MEM);

	mmu_enable();

	return 0;
}
mmu_initcall(mmu_init);

void mmu_disable(void)
{
	unsigned int cr;

	if (!ttb)
		arm_mmu_not_initialized_error();

	cr = get_cr();
	cr &= ~(CR_M | CR_C | CR_I);

	tlb_invalidate();

	dsb();
	isb();

	set_cr(cr);

	dsb();
	isb();
}

void mmu_early_enable(uint64_t membase, uint64_t memsize, uint64_t _ttb)
{
	ttb = (uint64_t *)_ttb;

	memset(ttb, 0, GRANULE_SIZE);

	set_ttbr_tcr_mair(current_el(), (uint64_t)ttb, TCR_FLAGS, MEMORY_ATTRIBUTES);

	create_sections(0, 0, 1UL << (BITS_PER_VA - 1), UNCACHED_MEM);

	create_sections(membase, membase, memsize, CACHED_MEM);

	isb();
	set_cr(get_cr() | CR_M);
}

unsigned long virt_to_phys(volatile void *virt)
{
	return (unsigned long)virt;
}

void *phys_to_virt(unsigned long phys)
{
	return (void *)phys;
}

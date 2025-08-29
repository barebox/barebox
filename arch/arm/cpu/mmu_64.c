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

static size_t granule_size(int level)
{
	/*
	 *  With 4k page granule, a virtual address is split into 4 lookup parts
	 *  spanning 9 bits each:
	 *
	 *    _______________________________________________
	 *   |       |       |       |       |       |       |
	 *   |   0   |  Lv0  |  Lv1  |  Lv2  |  Lv3  |  off  |
	 *   |_______|_______|_______|_______|_______|_______|
	 *     63-48   47-39   38-30   29-21   20-12   11-00
	 *
	 *             mask        page size
	 *
	 *    Lv0: FF8000000000       --
	 *    Lv1:   7FC0000000       1G
	 *    Lv2:     3FE00000       2M
	 *    Lv3:       1FF000       4K
	 *    off:          FFF
	 */
	switch (level) {
	default:
	case 0:
		return L0_XLAT_SIZE;
	case 1:
		return L1_XLAT_SIZE;
	case 2:
		return L2_XLAT_SIZE;
	case 3:
		return L3_XLAT_SIZE;
	}
}

static uint64_t *get_ttb(void)
{
	return (uint64_t *)get_ttbr(current_el());
}

static void set_pte(uint64_t *pt, uint64_t val)
{
	WRITE_ONCE(*pt, val);
}

static void set_pte_range(unsigned level, uint64_t *virt, phys_addr_t phys,
			  size_t count, uint64_t attrs, bool bbm)
{
	unsigned granularity = granule_size(level);
	if (!bbm)
		goto write_attrs;

	 // TODO break-before-make missing

write_attrs:
	for (int i = 0; i < count; i++, phys += granularity)
		set_pte(&virt[i], phys | attrs);

	dma_flush_range(virt, count * sizeof(*virt));
}

#ifdef __PBL__
static uint64_t *alloc_pte(void)
{
	static unsigned int idx;

	idx++;

	BUG_ON(idx * GRANULE_SIZE >= ARM_EARLY_PAGETABLE_SIZE);

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

/**
 * find_pte - Find page table entry
 * @ttb: Translation Table Base
 * @addr: Virtual address to lookup
 * @level: used to store the level at which the page table walk ended.
 *         if NULL, asserts that the smallest page was found
 *
 * This function walks the page table from the top down and finds the page
 * table entry associated with the supplied virtual address.
 * The level at which a page was found is saved into *level.
 * if the level is NULL, a last level page must be found or the function
 * panics.
 *
 * Returns a pointer to the page table entry
 */
static uint64_t *find_pte(uint64_t *ttb, uint64_t addr, unsigned *level)
{
	uint64_t *pte = ttb;
	uint64_t block_shift;
	uint64_t idx;
	int i;

	for (i = 0; i < 4; i++) {
		block_shift = level2shift(i);
		idx = (addr & level2mask(i)) >> block_shift;
		pte += idx;

		if ((pte_type(pte) != PTE_TYPE_TABLE) || (block_shift <= GRANULE_SIZE_SHIFT))
			break;
		else
			pte = (uint64_t *)(*pte & XLAT_ADDR_MASK);
	}

	if (!level && i != 3)
		panic("Got level %d page table entry, where level 3 expected\n", i);
	if (level)
		*level = i;
	return pte;
}

static unsigned long get_pte_attrs(maptype_t map_type)
{
	switch (map_type & MAP_TYPE_MASK) {
	case MAP_CACHED:
		return attrs_xn() | CACHED_MEM;
	case MAP_UNCACHED:
		return attrs_xn() | UNCACHED_MEM;
	case MAP_FAULT:
		return 0x0;
	case MAP_WRITECOMBINE:
		return attrs_xn() | MEM_ALLOC_WRITECOMBINE;
	case MAP_CODE:
		return CACHED_MEM | PTE_BLOCK_RO;
	case ARCH_MAP_CACHED_RO:
		return attrs_xn() | CACHED_MEM | PTE_BLOCK_RO;
	case ARCH_MAP_CACHED_RWX:
		return CACHED_MEM;
	default:
		return ~0UL;
	}
}

#define MAX_PTE_ENTRIES 512

/* Splits a block PTE into table with subpages spanning the old block */
static void split_block(uint64_t *pte, int level, bool bbm)
{
	uint64_t old_pte = *pte;
	uint64_t *new_table;
	u64 flags = 0;

	if ((*pte & PTE_TYPE_MASK) == PTE_TYPE_TABLE)
		return;

	new_table = alloc_pte();

	/* Level 3 block PTEs have the table type */
	if ((level + 1) == 3)
		flags |= PTE_TYPE_TABLE;

	set_pte_range(level + 1, new_table, old_pte, MAX_PTE_ENTRIES, flags, bbm);

	/* level describes the parent level, we need the child ones */
	set_pte_range(level, pte, (uint64_t)new_table, 1, PTE_TYPE_TABLE, bbm);
}

static int __arch_remap_range(uint64_t virt, uint64_t phys, uint64_t size,
			      maptype_t map_type, bool bbm)
{
	bool force_pages = map_type & ARCH_MAP_FLAG_PAGEWISE;
	unsigned long attr = get_pte_attrs(map_type);
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

	if (WARN_ON(attr == ~0UL))
		return -EINVAL;

	pr_debug_remap(addr, phys, size, map_type);

	attr &= ~PTE_TYPE_MASK;

	size = PAGE_ALIGN(size);
	if (!size)
		return 0;

	while (size) {
		table = ttb;
		for (level = 0; level < 4; level++) {
			bool block_aligned;
			block_shift = level2shift(level);
			idx = (addr & level2mask(level)) >> block_shift;
			block_size = (1ULL << block_shift);

			pte = table + idx;

			block_aligned = size >= block_size &&
				        IS_ALIGNED(addr, block_size) &&
				        IS_ALIGNED(phys, block_size);

			if ((force_pages && level == 3) || (!force_pages && block_aligned)) {
				type = (level == 3) ?
					PTE_TYPE_PAGE : PTE_TYPE_BLOCK;

				set_pte_range(level, pte, phys, 1, attr | type, bbm);
				addr += block_size;
				phys += block_size;
				size -= block_size;
				break;
			} else {
				split_block(pte, level, bbm);
			}

			table = get_level_table(pte);
		}

	}

	tlb_invalidate();
	return 0;
}

static bool pte_is_cacheable(uint64_t pte, int level)
{
	return (pte & PTE_ATTRINDX_MASK) == PTE_ATTRINDX(MT_NORMAL);
}

/**
 * dma_flush_range_end - Flush caches for address range
 * @start: Starting virtual address of the range.
 * @end:   Last virtual address in range (inclusive)
 *
 * This function cleans and invalidates all cache lines in the specified
 * range. Note that end is inclusive, meaning that it's the last address
 * that is flushed (assuming both start and total size are cache line aligned).
 */
static inline void dma_flush_range_end(unsigned long start, unsigned long end)
{
	v8_flush_dcache_range(start, end + 1);
}

#include "flush_cacheable_pages.h"

static void early_remap_range(uint64_t addr, size_t size, maptype_t map_type)
{
	__arch_remap_range(addr, addr, size, map_type, false);
}

int arch_remap_range(void *virt_addr, phys_addr_t phys_addr, size_t size, maptype_t map_type)
{
	map_type = arm_mmu_maybe_skip_permissions(map_type);

	if (!maptype_is_compatible(map_type, MAP_CACHED))
		flush_cacheable_pages(virt_addr, size);

	return __arch_remap_range((uint64_t)virt_addr, phys_addr, (uint64_t)size, map_type, true);
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
	request_barebox_region("guard page", guard_page, PAGE_SIZE,
			       MEMATTRS_FAULT);
	remap_range((void *)guard_page, PAGE_SIZE, MAP_FAULT);

	pr_debug("Created guard page\n");
}

void setup_trap_pages(void)
{
	/* Vectors are already registered by aarch64_init_vectors */
	/* Make zero page faulting to catch NULL pointer derefs */
	zero_page_faulting();
	create_guard_page();
}

/*
 * Prepare MMU for usage enable it.
 */
void __mmu_init(bool mmu_on)
{
	uint64_t *ttb = get_ttb();

	// TODO: remap writable only while remapping?
	// TODO: What memtype for ttb when barebox is EFI loader?
	if (!request_barebox_region("ttb", (unsigned long)ttb,
				  ARM_EARLY_PAGETABLE_SIZE, MEMATTRS_RW))
		/*
		 * This can mean that:
		 * - the early MMU code has put the ttb into a place
		 *   which we don't have inside our available memory
		 * - Somebody else has occupied the ttb region which means
		 *   the ttb will get corrupted.
		 */
		pr_crit("Can't request SDRAM region for ttb at %p\n", ttb);
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

static void early_init_range(size_t total_level0_tables)
{
	uint64_t *ttb = get_ttb();
	uint64_t addr = 0;

	while (total_level0_tables--) {
		early_remap_range(addr, L0_XLAT_SIZE, MAP_UNCACHED);
		split_block(ttb, 0, false);
		addr += L0_XLAT_SIZE;
		ttb++;
	}
}

void mmu_early_enable(unsigned long membase, unsigned long memsize, unsigned long barebox_start)
{
	int el;
	u64 optee_membase;
	unsigned long barebox_size;
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
	early_init_range(2);

	early_remap_range(membase, memsize, ARCH_MAP_CACHED_RWX);

	/* Default location for OP-TEE: end of DRAM, leave OPTEE_SIZE space for it */
	optee_membase = membase + memsize - OPTEE_SIZE;

	barebox_size = optee_membase - barebox_start;

	/*
	 * map barebox area using pagewise mapping. We want to modify the XN/RO
	 * attributes later, but can't switch from sections to pages later when
	 * executing code from it
	 */
	early_remap_range(barebox_start, barebox_size,
		     ARCH_MAP_CACHED_RWX | ARCH_MAP_FLAG_PAGEWISE);

	/* OP-TEE might be at location specified in OP-TEE header */
	optee_get_membase(&optee_membase);

	early_remap_range(optee_membase, OPTEE_SIZE, MAP_FAULT);

	early_remap_range(PAGE_ALIGN_DOWN((uintptr_t)_stext), PAGE_ALIGN(_etext - _stext),
			  ARCH_MAP_CACHED_RWX);

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

/*
 * Copyright (c) 2009-2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <asm/mmu.h>
#include <errno.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <memory.h>
#include <asm/system_info.h>

#include "mmu.h"

static unsigned long *ttb;

static void create_sections(unsigned long virt, unsigned long phys, int size_m,
		unsigned int flags)
{
	int i;

	phys >>= 20;
	virt >>= 20;

	for (i = size_m; i > 0; i--, virt++, phys++)
		ttb[virt] = (phys << 20) | flags;

	__mmu_cache_flush();
}

/*
 * Do it the simple way for now and invalidate the entire
 * tlb
 */
static inline void tlb_invalidate(void)
{
	asm volatile (
		"mov	r0, #0\n"
		"mcr	p15, 0, r0, c7, c10, 4;	@ drain write buffer\n"
		"mcr	p15, 0, r0, c8, c6, 0;  @ invalidate D TLBs\n"
		"mcr	p15, 0, r0, c8, c5, 0;  @ invalidate I TLBs\n"
		:
		:
		: "r0"
	);
}

#define PTE_FLAGS_CACHED_V7 (PTE_EXT_TEX(1) | PTE_BUFFERABLE | PTE_CACHEABLE)
#define PTE_FLAGS_WC_V7 PTE_EXT_TEX(1)
#define PTE_FLAGS_UNCACHED_V7 (0)
#define PTE_FLAGS_CACHED_V4 (PTE_SMALL_AP_UNO_SRW | PTE_BUFFERABLE | PTE_CACHEABLE)
#define PTE_FLAGS_UNCACHED_V4 PTE_SMALL_AP_UNO_SRW

/*
 * PTE flags to set cached and uncached areas.
 * This will be determined at runtime.
 */
static uint32_t pte_flags_cached;
static uint32_t pte_flags_wc;
static uint32_t pte_flags_uncached;

#define PTE_MASK ((1 << 12) - 1)

uint32_t mmu_get_pte_cached_flags()
{
	return pte_flags_cached;
}

uint32_t mmu_get_pte_uncached_flags()
{
	return pte_flags_uncached;
}

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
 * Create a second level translation table for the given virtual address.
 * We initially create a flat uncached mapping on it.
 * Not yet exported, but may be later if someone finds use for it.
 */
static u32 *arm_create_pte(unsigned long virt)
{
	u32 *table;
	int i;

	table = memalign(0x400, 0x400);

	if (!ttb)
		arm_mmu_not_initialized_error();

	ttb[virt >> 20] = (unsigned long)table | PMD_TYPE_TABLE;

	for (i = 0; i < 256; i++) {
		table[i] = virt | PTE_TYPE_SMALL | pte_flags_uncached;
		virt += PAGE_SIZE;
	}

	return table;
}

static u32 *find_pte(unsigned long adr)
{
	u32 *table;

	if (!ttb)
		arm_mmu_not_initialized_error();

	if ((ttb[adr >> 20] & PMD_TYPE_MASK) != PMD_TYPE_TABLE) {
		struct memory_bank *bank;
		int i = 0;

		/*
		 * This should only be called for page mapped memory inside our
		 * memory banks. It's a bug to call it with section mapped memory
		 * locations.
		 */
		pr_crit("%s: TTB for address 0x%08lx is not of type table\n",
				__func__, adr);
		pr_crit("Memory banks:\n");
		for_each_memory_bank(bank)
			pr_crit("#%d 0x%08lx - 0x%08lx\n", i, bank->start,
					bank->start + bank->size - 1);
		BUG();
	}

	/* find the coarse page table base address */
	table = (u32 *)(ttb[adr >> 20] & ~0x3ff);

	/* find second level descriptor */
	return &table[(adr >> PAGE_SHIFT) & 0xff];
}

static void dma_flush_range(unsigned long start, unsigned long end)
{
	__dma_flush_range(start, end);
	if (outer_cache.flush_range)
		outer_cache.flush_range(start, end);
}

static void dma_inv_range(unsigned long start, unsigned long end)
{
	if (outer_cache.inv_range)
		outer_cache.inv_range(start, end);
	__dma_inv_range(start, end);
}

void remap_range(void *_start, size_t size, uint32_t flags)
{
	unsigned long start = (unsigned long)_start;
	u32 *p;
	int numentries, i;

	numentries = size >> PAGE_SHIFT;
	p = find_pte(start);

	for (i = 0; i < numentries; i++) {
		p[i] &= ~PTE_MASK;
		p[i] |= flags | PTE_TYPE_SMALL;
	}

	dma_flush_range((unsigned long)p,
			(unsigned long)p + numentries * sizeof(u32));

	tlb_invalidate();
}

void *map_io_sections(unsigned long phys, void *_start, size_t size)
{
	unsigned long start = (unsigned long)_start, sec;

	phys >>= 20;
	for (sec = start; sec < start + size; sec += (1 << 20))
		ttb[sec >> 20] = (phys++ << 20) | PMD_SECT_DEF_UNCACHED;

	dma_flush_range((unsigned long)ttb, (unsigned long)ttb + 0x4000);
	tlb_invalidate();
	return _start;
}

/*
 * remap the memory bank described by mem cachable and
 * bufferable
 */
static int arm_mmu_remap_sdram(struct memory_bank *bank)
{
	unsigned long phys = (unsigned long)bank->start;
	unsigned long ttb_start = phys >> 20;
	unsigned long ttb_end = (phys >> 20) + (bank->size >> 20);
	unsigned long num_ptes = bank->size >> 10;
	int i, pte;
	u32 *ptes;

	pr_debug("remapping SDRAM from 0x%08lx (size 0x%08lx)\n",
			phys, bank->size);

	/*
	 * We replace each 1MiB section in this range with second level page
	 * tables, therefore we must have 1Mib aligment here.
	 */
	if ((phys & (SZ_1M - 1)) || (bank->size & (SZ_1M - 1)))
		return -EINVAL;

	ptes = xmemalign(PAGE_SIZE, num_ptes * sizeof(u32));

	pr_debug("ptes: 0x%p ttb_start: 0x%08lx ttb_end: 0x%08lx\n",
			ptes, ttb_start, ttb_end);

	for (i = 0; i < num_ptes; i++) {
		ptes[i] = (phys + i * PAGE_SIZE) | PTE_TYPE_SMALL |
			pte_flags_cached;
	}

	pte = 0;

	for (i = ttb_start; i < ttb_end; i++) {
		ttb[i] = (unsigned long)(&ptes[pte]) | PMD_TYPE_TABLE |
			(0 << 4);
		pte += 256;
	}

	dma_flush_range((unsigned long)ttb, (unsigned long)ttb + 0x4000);
	dma_flush_range((unsigned long)ptes, num_ptes * sizeof(u32));

	tlb_invalidate();

	return 0;
}
/*
 * We have 8 exception vectors and the table consists of absolute
 * jumps, so we need 8 * 4 bytes for the instructions and another
 * 8 * 4 bytes for the addresses.
 */
#define ARM_VECTORS_SIZE	(sizeof(u32) * 8 * 2)

/*
 * Map vectors and zero page
 */
static void vectors_init(void)
{
	u32 *exc, *zero = NULL;
	void *vectors;
	u32 cr;

	cr = get_cr();
	cr |= CR_V;
	set_cr(cr);
	cr = get_cr();

	if (cr & CR_V) {
		/*
		 * If we can use high vectors, create the second level
		 * page table for the high vectors and zero page
		 */
		exc = arm_create_pte(0xfff00000);
		zero = arm_create_pte(0x0);

		/* Set the zero page to faulting */
		zero[0] = 0;
	} else {
		/*
		 * Otherwise map the vectors to the zero page. We have to
		 * live without being able to catch NULL pointer dereferences
		 */
		exc = arm_create_pte(0x0);
	}

	arm_fixup_vectors();

	vectors = xmemalign(PAGE_SIZE, PAGE_SIZE);
	memset(vectors, 0, PAGE_SIZE);
	memcpy(vectors, __exceptions_start, __exceptions_stop - __exceptions_start);

	if (cr & CR_V)
		exc[256 - 16] = (u32)vectors | PTE_TYPE_SMALL |
			pte_flags_cached;
	else
		exc[0] = (u32)vectors | PTE_TYPE_SMALL | pte_flags_cached;
}

/*
 * Prepare MMU for usage enable it.
 */
static int mmu_init(void)
{
	struct memory_bank *bank;
	int i;

	if (list_empty(&memory_banks))
		/*
		 * If you see this it means you have no memory registered.
		 * This can be done either with arm_add_mem_device() in an
		 * initcall prior to mmu_initcall or via devicetree in the
		 * memory node.
		 */
		panic("MMU: No memory bank found! Cannot continue\n");

	arm_set_cache_functions();

	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		pte_flags_cached = PTE_FLAGS_CACHED_V7;
		pte_flags_wc = PTE_FLAGS_WC_V7;
		pte_flags_uncached = PTE_FLAGS_UNCACHED_V7;
	} else {
		pte_flags_cached = PTE_FLAGS_CACHED_V4;
		pte_flags_wc = PTE_FLAGS_UNCACHED_V4;
		pte_flags_uncached = PTE_FLAGS_UNCACHED_V4;
	}

	if (get_cr() & CR_M) {
		/*
		 * Early MMU code has already enabled the MMU. We assume a
		 * flat 1:1 section mapping in this case.
		 */
		asm volatile ("mrc  p15,0,%0,c2,c0,0" : "=r"(ttb));

		/* Clear unpredictable bits [13:0] */
		ttb = (unsigned long *)((unsigned long)ttb & ~0x3fff);

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
		ttb = memalign(0x10000, 0x4000);
	}

	pr_debug("ttb: 0x%p\n", ttb);

	/* Set the ttb register */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);

	/* Set the Domain Access Control Register */
	i = 0x3;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/* create a flat mapping using 1MiB sections */
	create_sections(0, 0, PAGE_SIZE, PMD_SECT_AP_WRITE | PMD_SECT_AP_READ |
			PMD_TYPE_SECT);

	vectors_init();

	/*
	 * First remap sdram cached using sections.
	 * This is to speed up the generation of 2nd level page tables
	 * below
	 */
	for_each_memory_bank(bank)
		create_sections(bank->start, bank->start, bank->size >> 20,
				PMD_SECT_DEF_CACHED);

	__mmu_cache_on();

	/*
	 * Now that we have the MMU and caches on remap sdram again using
	 * page tables
	 */
	for_each_memory_bank(bank)
		arm_mmu_remap_sdram(bank);

	return 0;
}
mmu_initcall(mmu_init);

void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	void *ret;

	size = PAGE_ALIGN(size);
	ret = xmemalign(PAGE_SIZE, size);
	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	dma_inv_range((unsigned long)ret, (unsigned long)ret + size);

	remap_range(ret, size, pte_flags_uncached);

	return ret;
}

void *dma_alloc_writecombine(size_t size, dma_addr_t *dma_handle)
{
	void *ret;

	size = PAGE_ALIGN(size);
	ret = xmemalign(PAGE_SIZE, size);
	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	dma_inv_range((unsigned long)ret, (unsigned long)ret + size);

	remap_range(ret, size, pte_flags_wc);

	return ret;
}

unsigned long virt_to_phys(volatile void *virt)
{
	return (unsigned long)virt;
}

void *phys_to_virt(unsigned long phys)
{
	return (void *)phys;
}

void dma_free_coherent(void *mem, dma_addr_t dma_handle, size_t size)
{
	size = PAGE_ALIGN(size);
	remap_range(mem, size, pte_flags_cached);

	free(mem);
}

void dma_sync_single_for_cpu(unsigned long address, size_t size,
			     enum dma_data_direction dir)
{
	if (dir != DMA_TO_DEVICE) {
		if (outer_cache.inv_range)
			outer_cache.inv_range(address, address + size);
		__dma_inv_range(address, address + size);
	}
}

void dma_sync_single_for_device(unsigned long address, size_t size,
				enum dma_data_direction dir)
{
	if (dir == DMA_FROM_DEVICE) {
		__dma_inv_range(address, address + size);
		if (outer_cache.inv_range)
			outer_cache.inv_range(address, address + size);
	} else {
		__dma_clean_range(address, address + size);
		if (outer_cache.clean_range)
			outer_cache.clean_range(address, address + size);
	}
}

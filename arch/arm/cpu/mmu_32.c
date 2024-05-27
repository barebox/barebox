// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2009-2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#define pr_fmt(fmt)	"mmu: " fmt

#include <common.h>
#include <dma-dir.h>
#include <dma.h>
#include <init.h>
#include <mmu.h>
#include <errno.h>
#include <zero_page.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <memory.h>
#include <asm/system_info.h>
#include <asm/sections.h>
#include <linux/pagemap.h>

#include "mmu_32.h"

#define PTRS_PER_PTE		(PGDIR_SIZE / PAGE_SIZE)
#define ARCH_MAP_WRITECOMBINE	((unsigned)-1)

static inline uint32_t *get_ttb(void)
{
	/* Clear unpredictable bits [13:0] */
	return (uint32_t *)(get_ttbr() & ~0x3fff);
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

#define PTE_FLAGS_CACHED_V7 (PTE_EXT_TEX(1) | PTE_BUFFERABLE | PTE_CACHEABLE | \
			     PTE_EXT_AP_URW_SRW)
#define PTE_FLAGS_WC_V7 (PTE_EXT_TEX(1) | PTE_EXT_AP_URW_SRW | PTE_EXT_XN)
#define PTE_FLAGS_UNCACHED_V7 (PTE_EXT_AP_URW_SRW | PTE_EXT_XN)
#define PTE_FLAGS_CACHED_V4 (PTE_SMALL_AP_UNO_SRW | PTE_BUFFERABLE | PTE_CACHEABLE)
#define PTE_FLAGS_UNCACHED_V4 PTE_SMALL_AP_UNO_SRW
#define PGD_FLAGS_WC_V7 (PMD_SECT_TEX(1) | PMD_SECT_DEF_UNCACHED | \
			 PMD_SECT_BUFFERABLE | PMD_SECT_XN)
#define PGD_FLAGS_UNCACHED_V7 (PMD_SECT_DEF_UNCACHED | PMD_SECT_XN)

static bool pgd_type_table(u32 pgd)
{
	return (pgd & PMD_TYPE_MASK) == PMD_TYPE_TABLE;
}

#define PTE_SIZE       (PTRS_PER_PTE * sizeof(u32))

#ifdef __PBL__
static uint32_t *alloc_pte(void)
{
	static unsigned int idx = 3;

	idx++;

	if (idx * PTE_SIZE >= ARM_EARLY_PAGETABLE_SIZE)
		return NULL;

	return get_ttb() + idx * PTE_SIZE;
}
#else
static uint32_t *alloc_pte(void)
{
	return xmemalign(PTE_SIZE, PTE_SIZE);
}
#endif

static u32 *find_pte(unsigned long adr)
{
	u32 *table;
	uint32_t *ttb = get_ttb();

	if (!pgd_type_table(ttb[pgd_index(adr)]))
		return NULL;

	/* find the coarse page table base address */
	table = (u32 *)(ttb[pgd_index(adr)] & ~0x3ff);

	/* find second level descriptor */
	return &table[(adr >> PAGE_SHIFT) & 0xff];
}

void dma_flush_range(void *ptr, size_t size)
{
	unsigned long start = (unsigned long)ptr;
	unsigned long end = start + size;

	__dma_flush_range(start, end);

	if (outer_cache.flush_range)
		outer_cache.flush_range(start, end);
}

void dma_inv_range(void *ptr, size_t size)
{
	unsigned long start = (unsigned long)ptr;
	unsigned long end = start + size;

	if (outer_cache.inv_range)
		outer_cache.inv_range(start, end);

	__dma_inv_range(start, end);
}

/*
 * Create a second level translation table for the given virtual address.
 * We initially create a flat uncached mapping on it.
 * Not yet exported, but may be later if someone finds use for it.
 */
static u32 *arm_create_pte(unsigned long virt, unsigned long phys,
			   uint32_t flags)
{
	uint32_t *ttb = get_ttb();
	u32 *table;
	int i, ttb_idx;

	virt = ALIGN_DOWN(virt, PGDIR_SIZE);
	phys = ALIGN_DOWN(phys, PGDIR_SIZE);

	table = alloc_pte();

	ttb_idx = pgd_index(virt);

	for (i = 0; i < PTRS_PER_PTE; i++) {
		table[i] = phys | PTE_TYPE_SMALL | flags;
		virt += PAGE_SIZE;
		phys += PAGE_SIZE;
	}
	dma_flush_range(table, PTRS_PER_PTE * sizeof(u32));

	ttb[ttb_idx] = (unsigned long)table | PMD_TYPE_TABLE;
	dma_flush_range(&ttb[ttb_idx], sizeof(u32));

	return table;
}

static u32 pmd_flags_to_pte(u32 pmd)
{
	u32 pte = 0;

	if (pmd & PMD_SECT_BUFFERABLE)
		pte |= PTE_BUFFERABLE;
	if (pmd & PMD_SECT_CACHEABLE)
		pte |= PTE_CACHEABLE;

	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		if (pmd & PMD_SECT_nG)
			pte |= PTE_EXT_NG;
		if (pmd & PMD_SECT_XN)
			pte |= PTE_EXT_XN;

		/* TEX[2:0] */
		pte |= PTE_EXT_TEX((pmd >> 12) & 7);
		/* AP[1:0] */
		pte |= ((pmd >> 10) & 0x3) << 4;
		/* AP[2] */
		pte |= ((pmd >> 15) & 0x1) << 9;
	} else {
		pte |= PTE_SMALL_AP_UNO_SRW;
	}

	return pte;
}

static u32 pte_flags_to_pmd(u32 pte)
{
	u32 pmd = 0;

	if (pte & PTE_BUFFERABLE)
		pmd |= PMD_SECT_BUFFERABLE;
	if (pte & PTE_CACHEABLE)
		pmd |= PMD_SECT_CACHEABLE;

	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		if (pte & PTE_EXT_NG)
			pmd |= PMD_SECT_nG;
		if (pte & PTE_EXT_XN)
			pmd |= PMD_SECT_XN;

		/* TEX[2:0] */
		pmd |= ((pte >> 6) & 7) << 12;
		/* AP[1:0] */
		pmd |= ((pte >> 4) & 0x3) << 10;
		/* AP[2] */
		pmd |= ((pte >> 9) & 0x1) << 15;
	} else {
		pmd |= PMD_SECT_AP_WRITE | PMD_SECT_AP_READ;
	}

	return pmd;
}

static uint32_t get_pte_flags(int map_type)
{
	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		switch (map_type) {
		case MAP_CACHED:
			return PTE_FLAGS_CACHED_V7;
		case MAP_UNCACHED:
			return PTE_FLAGS_UNCACHED_V7;
		case ARCH_MAP_WRITECOMBINE:
			return PTE_FLAGS_WC_V7;
		case MAP_FAULT:
		default:
			return 0x0;
		}
	} else {
		switch (map_type) {
		case MAP_CACHED:
			return PTE_FLAGS_CACHED_V4;
		case MAP_UNCACHED:
		case ARCH_MAP_WRITECOMBINE:
			return PTE_FLAGS_UNCACHED_V4;
		case MAP_FAULT:
		default:
			return 0x0;
		}
	}
}

static uint32_t get_pmd_flags(int map_type)
{
	return pte_flags_to_pmd(get_pte_flags(map_type));
}

static void __arch_remap_range(void *_virt_addr, phys_addr_t phys_addr, size_t size, unsigned map_type)
{
	u32 virt_addr = (u32)_virt_addr;
	u32 pte_flags, pmd_flags;
	uint32_t *ttb = get_ttb();

	BUG_ON(!IS_ALIGNED(virt_addr, PAGE_SIZE));
	BUG_ON(!IS_ALIGNED(phys_addr, PAGE_SIZE));

	pte_flags = get_pte_flags(map_type);
	pmd_flags = pte_flags_to_pmd(pte_flags);

	size = PAGE_ALIGN(size);

	while (size) {
		const bool pgdir_size_aligned = IS_ALIGNED(virt_addr, PGDIR_SIZE);
		u32 *pgd = (u32 *)&ttb[pgd_index(virt_addr)];
		size_t chunk;

		if (size >= PGDIR_SIZE && pgdir_size_aligned &&
		    IS_ALIGNED(phys_addr, PGDIR_SIZE) &&
		    !pgd_type_table(*pgd)) {
			/*
			 * TODO: Add code to discard a page table and
			 * replace it with a section
			 */
			chunk = PGDIR_SIZE;
			*pgd = phys_addr | pmd_flags;
			if (map_type != MAP_FAULT)
				*pgd |= PMD_TYPE_SECT;
			dma_flush_range(pgd, sizeof(*pgd));
		} else {
			unsigned int num_ptes;
			u32 *table = NULL;
			unsigned int i;
			u32 *pte;
			/*
			 * We only want to cover pages up until next
			 * section boundary in case there we would
			 * have an opportunity to re-map the whole
			 * section (say if we got here becasue address
			 * was not aligned on PGDIR_SIZE boundary)
			 */
			chunk = pgdir_size_aligned ?
				PGDIR_SIZE : ALIGN(virt_addr, PGDIR_SIZE) - virt_addr;
			/*
			 * At the same time we want to make sure that
			 * we don't go on remapping past requested
			 * size in case that is less that the distance
			 * to next PGDIR_SIZE boundary.
			 */
			chunk = min(chunk, size);
			num_ptes = chunk / PAGE_SIZE;

			pte = find_pte(virt_addr);
			if (!pte) {
				/*
				 * If PTE is not found it means that
				 * we needs to split this section and
				 * create a new page table for it
				 */
				table = arm_create_pte(virt_addr, phys_addr,
						       pmd_flags_to_pte(*pgd));
				pte = find_pte(virt_addr);
				BUG_ON(!pte);
			}

			for (i = 0; i < num_ptes; i++) {
				pte[i] = phys_addr + i * PAGE_SIZE;
				pte[i] |= pte_flags;
				if (map_type != MAP_FAULT)
					pte[i] |= PTE_TYPE_SMALL;
			}

			dma_flush_range(pte, num_ptes * sizeof(u32));
		}

		virt_addr += chunk;
		phys_addr += chunk;
		size -= chunk;
	}

	tlb_invalidate();
}
static void early_remap_range(u32 addr, size_t size, unsigned map_type)
{
	__arch_remap_range((void *)addr, addr, size, map_type);
}

int arch_remap_range(void *virt_addr, phys_addr_t phys_addr, size_t size, unsigned map_type)
{
	__arch_remap_range(virt_addr, phys_addr, size, map_type);

	if (map_type == MAP_UNCACHED)
		dma_inv_range(virt_addr, size);

	return 0;
}

static void create_sections(unsigned long first, unsigned long last,
			    unsigned int flags)
{
	uint32_t *ttb = get_ttb();
	unsigned long ttb_start = pgd_index(first);
	unsigned long ttb_end = pgd_index(last) + 1;
	unsigned int i, addr = first;

	for (i = ttb_start; i < ttb_end; i++) {
		ttb[i] = addr | flags;
		addr += PGDIR_SIZE;
	}
}

static inline void create_flat_mapping(void)
{
	/* create a flat mapping using 1MiB sections */
	create_sections(0, 0xffffffff, attrs_uncached_mem());
}

void *map_io_sections(unsigned long phys, void *_start, size_t size)
{
	unsigned long start = (unsigned long)_start, sec;
	uint32_t *ttb = get_ttb();

	for (sec = start; sec < start + size; sec += PGDIR_SIZE, phys += PGDIR_SIZE)
		ttb[pgd_index(sec)] = phys | get_pmd_flags(MAP_UNCACHED);

	dma_flush_range(ttb, 0x4000);
	tlb_invalidate();
	return _start;
}

#define ARM_HIGH_VECTORS	0xffff0000
#define ARM_LOW_VECTORS		0x0

/**
 * create_vector_table - create a vector table at given address
 * @adr - The address where the vector table should be created
 *
 * After executing this function the vector table is found at the
 * virtual address @adr.
 */
static void create_vector_table(unsigned long adr)
{
	struct resource *vectors_sdram;
	void *vectors;
	u32 *pte;

	vectors_sdram = request_barebox_region("vector table", adr, PAGE_SIZE);
	if (vectors_sdram) {
		/*
		 * The vector table address is inside the SDRAM physical
		 * address space. Use the existing identity mapping for
		 * the vector table.
		 */
		pr_debug("Creating vector table, virt = phys = 0x%08lx\n", adr);
		vectors = (void *)vectors_sdram->start;
	} else {
		/*
		 * The vector table address is outside of SDRAM. Create
		 * a secondary page table for the section and map
		 * allocated memory to the vector address.
		 */
		vectors = xmemalign(PAGE_SIZE, PAGE_SIZE);
		pr_debug("Creating vector table, virt = 0x%p, phys = 0x%08lx\n",
			 vectors, adr);
		arm_create_pte(adr, adr, get_pte_flags(MAP_UNCACHED));
		pte = find_pte(adr);
		*pte = (u32)vectors | PTE_TYPE_SMALL | get_pte_flags(MAP_CACHED);
	}

	arm_fixup_vectors();

	memset(vectors, 0, PAGE_SIZE);
	memcpy(vectors, __exceptions_start, __exceptions_stop - __exceptions_start);
}

/**
 * set_vector_table - let CPU use the vector table at given address
 * @adr - The address of the vector table
 *
 * Depending on the CPU the possibilities differ. ARMv7 and later allow
 * to map the vector table to arbitrary addresses. Other CPUs only allow
 * vectors at 0xffff0000 or at 0x0.
 */
static int set_vector_table(unsigned long adr)
{
	u32 cr;

	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		pr_debug("Vectors are at 0x%08lx\n", adr);
		set_vbar(adr);
		return 0;
	}

	if (adr == ARM_HIGH_VECTORS) {
		cr = get_cr();
		cr |= CR_V;
		set_cr(cr);
		cr = get_cr();
		if (cr & CR_V) {
			pr_debug("Vectors are at 0x%08lx\n", adr);
			return 0;
		} else {
			return -EINVAL;
		}
	}

	if (adr == ARM_LOW_VECTORS) {
		cr = get_cr();
		cr &= ~CR_V;
		set_cr(cr);
		cr = get_cr();
		if (cr & CR_V) {
			return -EINVAL;
		} else {
			pr_debug("Vectors are at 0x%08lx\n", adr);
			return 0;
		}
	}

	return -EINVAL;
}

static void create_zero_page(void)
{
	/*
	 * In case the zero page is in SDRAM request it to prevent others
	 * from using it
	 */
	request_sdram_region("zero page", 0x0, PAGE_SIZE);

	zero_page_faulting();
	pr_debug("Created zero page\n");
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
 * Map vectors and zero page
 */
static void vectors_init(void)
{
	create_guard_page();

	/*
	 * First try to use the vectors where they actually are, works
	 * on ARMv7 and later.
	 */
	if (!set_vector_table((unsigned long)__exceptions_start)) {
		arm_fixup_vectors();
		create_zero_page();
		return;
	}

	/*
	 * Next try high vectors at 0xffff0000.
	 */
	if (!set_vector_table(ARM_HIGH_VECTORS)) {
		create_zero_page();
		create_vector_table(ARM_HIGH_VECTORS);
		return;
	}

	/*
	 * As a last resort use low vectors at 0x0. With this we can't
	 * set the zero page to faulting and can't catch NULL pointer
	 * exceptions.
	 */
	set_vector_table(ARM_LOW_VECTORS);
	create_vector_table(ARM_LOW_VECTORS);
}

/*
 * Prepare MMU for usage enable it.
 */
void __mmu_init(bool mmu_on)
{
	struct memory_bank *bank;
	uint32_t *ttb = get_ttb();

	if (!request_barebox_region("ttb", (unsigned long)ttb,
				    ARM_EARLY_PAGETABLE_SIZE))
		/*
		 * This can mean that:
		 * - the early MMU code has put the ttb into a place
		 *   which we don't have inside our available memory
		 * - Somebody else has occupied the ttb region which means
		 *   the ttb will get corrupted.
		 */
		pr_crit("Critical Error: Can't request SDRAM region for ttb at %p\n",
					ttb);

	pr_debug("ttb: 0x%p\n", ttb);

	/*
	 * Early mmu init will have mapped everything but the initial memory area
	 * (excluding final OPTEE_SIZE bytes) uncached. We have now discovered
	 * all memory banks, so let's map all pages, excluding reserved memory areas,
	 * cacheable and executable.
	 */
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

	vectors_init();
}

/*
 * Clean and invalide caches, disable MMU
 */
void mmu_disable(void)
{
	__mmu_cache_flush();
	if (outer_cache.disable) {
		outer_cache.flush_all();
		outer_cache.disable();
	}
	__mmu_cache_off();
}

void *dma_alloc_writecombine(size_t size, dma_addr_t *dma_handle)
{
	return dma_alloc_map(size, dma_handle, ARCH_MAP_WRITECOMBINE);
}

void mmu_early_enable(unsigned long membase, unsigned long memsize)
{
	uint32_t *ttb = (uint32_t *)arm_mem_ttb(membase + memsize);

	pr_debug("enabling MMU, ttb @ 0x%p\n", ttb);

	if (get_cr() & CR_M)
		return;

	set_ttbr(ttb);

	/* For the XN bit to take effect, we can't be using DOMAIN_MANAGER. */
	if (cpu_architecture() >= CPU_ARCH_ARMv7)
		set_domain(DOMAIN_CLIENT);
	else
		set_domain(DOMAIN_MANAGER);

	/*
	 * This marks the whole address space as uncachable as well as
	 * unexecutable if possible
	 */
	create_flat_mapping();

	/* maps main memory as cachable */
	early_remap_range(membase, memsize - OPTEE_SIZE, MAP_CACHED);
	early_remap_range(membase + memsize - OPTEE_SIZE, OPTEE_SIZE, MAP_UNCACHED);
	early_remap_range(PAGE_ALIGN_DOWN((uintptr_t)_stext), PAGE_ALIGN(_etext - _stext), MAP_CACHED);

	__mmu_cache_on();
}

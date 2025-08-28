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

static size_t granule_size(int level)
{
	/*
	 *  With 4k page granule, a virtual address is split into 2 lookup parts.
	 *  We don't do LPAE or large (64K) pages for ARM32.
	 *
	 *    _______________________
	 *   |       |       |       |
	 *   |  Lv1  |  Lv2  |  off  |
	 *   |_______|_______|_______|
	 *     31-21   20-12   11-00
	 *
	 *             mask        page size   term
	 *
	 *    Lv0:     E0000000       --
	 *    Lv1:     1FE00000       1M      PGD/PMD
	 *    Lv2:       1FF000       4K      PTE
	 *    off:          FFF
	 */

	switch (level) {
	case 1:
		return PGDIR_SIZE;
	case 2:
		return PAGE_SIZE;
	}

	return 0;
}

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

#define PTE_FLAGS_CACHED_V7_RWX (PTE_EXT_TEX(1) | PTE_BUFFERABLE | PTE_CACHEABLE | \
				 PTE_EXT_AP_URW_SRW)
#define PTE_FLAGS_CACHED_V7 (PTE_EXT_TEX(1) | PTE_BUFFERABLE | PTE_CACHEABLE | \
			     PTE_EXT_AP_URW_SRW | PTE_EXT_XN)
#define PTE_FLAGS_CACHED_RO_V7 (PTE_EXT_TEX(1) | PTE_BUFFERABLE | PTE_CACHEABLE | \
			     PTE_EXT_APX | PTE_EXT_AP0 | PTE_EXT_AP1 | PTE_EXT_XN)
#define PTE_FLAGS_CODE_V7 (PTE_EXT_TEX(1) | PTE_BUFFERABLE | PTE_CACHEABLE | \
			     PTE_EXT_APX | PTE_EXT_AP0 | PTE_EXT_AP1)
#define PTE_FLAGS_WC_V7 (PTE_EXT_TEX(1) | PTE_EXT_AP_URW_SRW | PTE_EXT_XN)
#define PTE_FLAGS_UNCACHED_V7 (PTE_EXT_AP_URW_SRW | PTE_EXT_XN)
#define PTE_FLAGS_CACHED_V4 (PTE_SMALL_AP_UNO_SRW | PTE_BUFFERABLE | PTE_CACHEABLE)
#define PTE_FLAGS_CACHED_RO_V4 (PTE_SMALL_AP_UNO_SRO | PTE_CACHEABLE)
#define PTE_FLAGS_UNCACHED_V4 PTE_SMALL_AP_UNO_SRW
#define PGD_FLAGS_WC_V7 (PMD_SECT_TEX(1) | PMD_SECT_DEF_UNCACHED | \
			 PMD_SECT_BUFFERABLE | PMD_SECT_XN)
#define PGD_FLAGS_UNCACHED_V7 (PMD_SECT_DEF_UNCACHED | PMD_SECT_XN)

static bool pgd_type_table(u32 pgd)
{
	return (pgd & PMD_TYPE_MASK) == PMD_TYPE_TABLE;
}

#define PTE_SIZE       (PTRS_PER_PTE * sizeof(u32))

static void set_pte(uint32_t *pt, uint32_t val)
{
	WRITE_ONCE(*pt, val);
}

static void set_pte_range(unsigned level, uint32_t *virt, phys_addr_t phys,
			  size_t count, uint32_t attrs, bool bbm)
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
static uint32_t *alloc_pte(void)
{
	static unsigned int idx = 3;

	idx++;

	BUG_ON(idx * PTE_SIZE >= ARM_EARLY_PAGETABLE_SIZE);

	return get_ttb() + idx * PTE_SIZE;
}
#else
static uint32_t *alloc_pte(void)
{
	return xmemalign(PTE_SIZE, PTE_SIZE);
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
static u32 *find_pte(uint32_t *ttb, uint32_t adr, unsigned *level)
{
	u32 *pgd = &ttb[pgd_index(adr)];
	u32 *table;

	if (!pgd_type_table(*pgd)) {
		if (!level)
			panic("Got level 1 page table entry, where level 2 expected\n");
		*level = 1;
		return pgd;
	}

	if (level)
		*level = 2;

	/* find the coarse page table base address */
	table = (u32 *)(*pgd & ~0x3ff);

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

/**
 * dma_flush_range_end - Flush caches for address range
 * @start: Starting virtual address of the range.
 * @end:   Last virtual address in range (inclusive)
 *
 * This function cleans and invalidates all cache lines in the specified
 * range. Note that end is inclusive, meaning that it's the last address
 * that is flushed (assuming both start and total size are cache line aligned).
 */
static void dma_flush_range_end(unsigned long start, unsigned long end)
{
	dma_flush_range((void *)start, end - start + 1);
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
			   uint32_t flags, bool bbm)
{
	uint32_t *ttb = get_ttb();
	u32 *table;
	int ttb_idx;

	virt = ALIGN_DOWN(virt, PGDIR_SIZE);
	phys = ALIGN_DOWN(phys, PGDIR_SIZE);

	table = alloc_pte();

	ttb_idx = pgd_index(virt);

	set_pte_range(2, table, phys, PTRS_PER_PTE, PTE_TYPE_SMALL | flags, bbm);

	set_pte_range(1, &ttb[ttb_idx], (unsigned long)table, 1, PMD_TYPE_TABLE, bbm);

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
		pmd |= PMD_SECT_AP_READ;
		if (pte & PTE_SMALL_AP_MASK)
			pmd |= PMD_SECT_AP_WRITE;
	}

	return pmd;
}

static uint32_t get_pte_flags(maptype_t map_type)
{
	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		switch (map_type & MAP_TYPE_MASK) {
		case ARCH_MAP_CACHED_RWX:
			return PTE_FLAGS_CACHED_V7_RWX;
		case ARCH_MAP_CACHED_RO:
			return PTE_FLAGS_CACHED_RO_V7;
		case MAP_CACHED:
			return PTE_FLAGS_CACHED_V7;
		case MAP_UNCACHED:
			return PTE_FLAGS_UNCACHED_V7;
		case MAP_CODE:
			return PTE_FLAGS_CODE_V7;
		case MAP_WRITECOMBINE:
			return PTE_FLAGS_WC_V7;
		case MAP_FAULT:
		default:
			return 0x0;
		}
	} else {
		switch (map_type & MAP_TYPE_MASK) {
		case ARCH_MAP_CACHED_RO:
		case MAP_CODE:
			return PTE_FLAGS_CACHED_RO_V4;
		case ARCH_MAP_CACHED_RWX:
		case MAP_CACHED:
			return PTE_FLAGS_CACHED_V4;
		case MAP_UNCACHED:
		case MAP_WRITECOMBINE:
			return PTE_FLAGS_UNCACHED_V4;
		case MAP_FAULT:
		default:
			return 0x0;
		}
	}
}

static uint32_t get_pmd_flags(maptype_t map_type)
{
	return pte_flags_to_pmd(get_pte_flags(map_type));
}

static void __arch_remap_range(void *_virt_addr, phys_addr_t phys_addr, size_t size,
			       maptype_t map_type)
{
	bool force_pages = map_type & ARCH_MAP_FLAG_PAGEWISE;
	bool mmu_on;
	u32 virt_addr = (u32)_virt_addr;
	u32 pte_flags, pmd_flags;
	uint32_t *ttb = get_ttb();

	BUG_ON(!IS_ALIGNED(virt_addr, PAGE_SIZE));
	BUG_ON(!IS_ALIGNED(phys_addr, PAGE_SIZE));

	pte_flags = get_pte_flags(map_type);
	pmd_flags = pte_flags_to_pmd(pte_flags);

	pr_debug_remap(virt_addr, phys_addr, size, map_type);

	size = PAGE_ALIGN(size);
	if (!size)
		return;

	mmu_on = get_cr() & CR_M;

	while (size) {
		const bool pgdir_size_aligned = IS_ALIGNED(virt_addr, PGDIR_SIZE);
		u32 *pgd = (u32 *)&ttb[pgd_index(virt_addr)];
		u32 flags;
		size_t chunk;

		if (size >= PGDIR_SIZE && pgdir_size_aligned &&
		    IS_ALIGNED(phys_addr, PGDIR_SIZE) &&
		    !pgd_type_table(*pgd) && !force_pages) {
			/*
			 * TODO: Add code to discard a page table and
			 * replace it with a section
			 */
			chunk = PGDIR_SIZE;
			flags = pmd_flags;
			if (!maptype_is_compatible(map_type, MAP_FAULT))
				flags |= PMD_TYPE_SECT;
			set_pte_range(1, pgd, phys_addr, 1, flags, mmu_on);
		} else {
			unsigned int num_ptes;
			u32 *table = NULL;
			unsigned int level;
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

			pte = find_pte(ttb, virt_addr, &level);
			if (level == 1) {
				/*
				 * No PTE at level 2, so we needs to split this section
				 * and create a new page table for it
				 */
				table = arm_create_pte(virt_addr, phys_addr,
						       pmd_flags_to_pte(*pgd), mmu_on);
				pte = find_pte(ttb, virt_addr, NULL);
			}

			flags = pte_flags;
			if (!maptype_is_compatible(map_type, MAP_FAULT))
				flags |= PTE_TYPE_SMALL;
			set_pte_range(2, pte, phys_addr, num_ptes, flags, mmu_on);
		}

		virt_addr += chunk;
		phys_addr += chunk;
		size -= chunk;
	}

	tlb_invalidate();
}

static void early_remap_range(u32 addr, size_t size, maptype_t map_type)
{
	__arch_remap_range((void *)addr, addr, size, map_type);
}

static bool pte_is_cacheable(uint32_t pte, int level)
{
	return	(level == 2 && (pte & PTE_CACHEABLE)) ||
		(level == 1 && (pte & PMD_SECT_CACHEABLE));
}

#include "flush_cacheable_pages.h"

int arch_remap_range(void *virt_addr, phys_addr_t phys_addr, size_t size, maptype_t map_type)
{
	if (!maptype_is_compatible(map_type, MAP_CACHED))
		flush_cacheable_pages(virt_addr, size);

	map_type = arm_mmu_maybe_skip_permissions(map_type);

	__arch_remap_range(virt_addr, phys_addr, size, map_type);

	return 0;
}

static void early_create_sections(unsigned long first, unsigned long last,
				  unsigned int flags)
{
	uint32_t *ttb = get_ttb();
	unsigned long ttb_start = pgd_index(first);
	unsigned long ttb_end = pgd_index(last) + 1;
	unsigned int i, addr = first;

	/* This always runs with MMU disabled, so just opencode the loop */
	for (i = ttb_start; i < ttb_end; i++) {
		set_pte(&ttb[i], addr | flags);
		addr += PGDIR_SIZE;
	}
}

static inline void early_create_flat_mapping(void)
{
	/* create a flat mapping using 1MiB sections */
	early_create_sections(0, 0xffffffff, attrs_uncached_mem());
}

void *map_io_sections(unsigned long phys, void *_start, size_t size)
{
	unsigned long start = (unsigned long)_start;
	uint32_t *ttb = get_ttb();

	set_pte_range(1, &ttb[pgd_index(start)], phys, size / PGDIR_SIZE,
		      get_pmd_flags(MAP_UNCACHED), true);

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

	vectors_sdram = request_barebox_region("vector table", adr, PAGE_SIZE,
					       MEMATTRS_RWX); // FIXME
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

		arm_create_pte(adr, adr, get_pte_flags(MAP_UNCACHED), true);
		pte = find_pte(get_ttb(), adr, NULL);
		set_pte_range(2, pte, (u32)vectors, 1, PTE_TYPE_SMALL |
			      get_pte_flags(MAP_CACHED), true);
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
	request_sdram_region("zero page", 0x0, PAGE_SIZE,
			     MEMTYPE_BOOT_SERVICES_DATA, MEMATTRS_FAULT);

	zero_page_faulting();
	pr_debug("Created zero page\n");
}

static void create_guard_page(void)
{
	ulong guard_page;

	if (!IS_ENABLED(CONFIG_STACK_GUARD_PAGE))
		return;

	guard_page = arm_mem_guard_page_get();
	request_barebox_region("guard page", guard_page, PAGE_SIZE, MEMATTRS_FAULT);
	remap_range((void *)guard_page, PAGE_SIZE, MAP_FAULT);

	pr_debug("Created guard page\n");
}

/*
 * Map vectors and zero page
 */
void setup_trap_pages(void)
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
	uint32_t *ttb = get_ttb();

	// TODO: remap writable only while remapping?
	// TODO: What memtype for ttb when barebox is EFI loader?
	if (!request_barebox_region("ttb", (unsigned long)ttb,
				    ARM_EARLY_PAGETABLE_SIZE,
				    MEMATTRS_RW))
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

void mmu_early_enable(unsigned long membase, unsigned long memsize, unsigned long barebox_start)
{
	uint32_t *ttb = (uint32_t *)arm_mem_ttb(membase + memsize);
	unsigned long barebox_size, optee_start;

	pr_debug("enabling MMU, ttb @ 0x%p\n", ttb);

	if (get_cr() & CR_M)
		return;

	set_ttbr(ttb);

	set_domain(DOMAIN_CLIENT);

	/*
	 * This marks the whole address space as uncachable as well as
	 * unexecutable if possible
	 */
	early_create_flat_mapping();

	/* maps main memory as cachable */
	optee_start = membase + memsize - OPTEE_SIZE;
	barebox_size = optee_start - barebox_start;

	/*
	 * map the bulk of the memory as sections to avoid allocating too many page tables
	 * at this early stage
	 */
	early_remap_range(membase, barebox_start - membase, ARCH_MAP_CACHED_RWX);
	/*
	 * Map the remainder of the memory explicitly with two level page tables. This is
	 * the place where barebox proper ends at. In barebox proper we'll remap the code
	 * segments readonly/executable and the ro segments readonly/execute never. For this
	 * we need the memory being mapped pagewise. We can't do the split up from section
	 * wise mapping to pagewise mapping later because that would require us to do
	 * a break-before-make sequence which we can't do when barebox proper is running
	 * at the location being remapped.
	 */
	early_remap_range(barebox_start, barebox_size,
			  ARCH_MAP_CACHED_RWX | ARCH_MAP_FLAG_PAGEWISE);
	early_remap_range(optee_start, OPTEE_SIZE, MAP_UNCACHED);
	early_remap_range(PAGE_ALIGN_DOWN((uintptr_t)_stext), PAGE_ALIGN(_etext - _stext),
			  ARCH_MAP_CACHED_RWX);

	__mmu_cache_on();
}

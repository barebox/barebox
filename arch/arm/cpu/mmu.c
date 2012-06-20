#include <common.h>
#include <init.h>
#include <asm/mmu.h>
#include <errno.h>
#include <sizes.h>
#include <asm/memory.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <memory.h>

static unsigned long *ttb;

static void create_sections(unsigned long virt, unsigned long phys, int size_m,
		unsigned int flags)
{
	int i;

	phys >>= 20;
	virt >>= 20;

	for (i = size_m; i > 0; i--, virt++, phys++)
		ttb[virt] = (phys << 20) | flags;

	asm volatile (
		"bl __mmu_cache_flush;"
		:
		:
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "lr", "cc", "memory"
	);
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

#ifdef CONFIG_CPU_V7
#define PTE_FLAGS_CACHED (PTE_EXT_TEX(1) | PTE_BUFFERABLE | PTE_CACHEABLE)
#define PTE_FLAGS_UNCACHED (0)
#else
#define PTE_FLAGS_CACHED (PTE_SMALL_AP_UNO_SRW | PTE_BUFFERABLE | PTE_CACHEABLE)
#define PTE_FLAGS_UNCACHED PTE_SMALL_AP_UNO_SRW
#endif

#define PTE_MASK ((1 << 12) - 1)

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

	ttb[virt >> 20] = (unsigned long)table | PMD_TYPE_TABLE;

	for (i = 0; i < 256; i++) {
		table[i] = virt | PTE_TYPE_SMALL | PTE_FLAGS_UNCACHED;
		virt += PAGE_SIZE;
	}

	return table;
}

static u32 *find_pte(unsigned long adr)
{
	u32 *table;

	if ((ttb[adr >> 20] & PMD_TYPE_MASK) != PMD_TYPE_TABLE)
		BUG();

	/* find the coarse page table base address */
	table = (u32 *)(ttb[adr >> 20] & ~0x3ff);

	/* find second level descriptor */
	return &table[(adr >> PAGE_SHIFT) & 0xff];
}

static void remap_range(void *_start, size_t size, uint32_t flags)
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
	unsigned long ttb_end = (phys + bank->size) >> 20;
	unsigned long num_ptes = bank->size >> 10;
	int i, pte;
	u32 *ptes;

	debug("remapping SDRAM from 0x%08lx (size 0x%08lx)\n",
			phys, bank->size);

	/*
	 * We replace each 1MiB section in this range with second level page
	 * tables, therefore we must have 1Mib aligment here.
	 */
	if ((phys & (SZ_1M - 1)) || (bank->size & (SZ_1M - 1)))
		return -EINVAL;

	ptes = memalign(0x400, num_ptes * sizeof(u32));

	debug("ptes: 0x%p ttb_start: 0x%08lx ttb_end: 0x%08lx\n",
			ptes, ttb_start, ttb_end);

	for (i = 0; i < num_ptes; i++) {
		ptes[i] = (phys + i * 4096) | PTE_TYPE_SMALL |
			PTE_FLAGS_CACHED;
	}

	pte = 0;

	for (i = ttb_start; i < ttb_end; i++) {
		ttb[i] = (unsigned long)(&ptes[pte]) | PMD_TYPE_TABLE |
			(0 << 4);
		pte += 256;
	}

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

	vectors = xmemalign(PAGE_SIZE, PAGE_SIZE);
	memset(vectors, 0, PAGE_SIZE);
	memcpy(vectors, __exceptions_start, __exceptions_stop - __exceptions_start);

	if (cr & CR_V)
		exc[256 - 16] = (u32)vectors | PTE_TYPE_SMALL | PTE_FLAGS_CACHED;
	else
		exc[0] = (u32)vectors | PTE_TYPE_SMALL | PTE_FLAGS_CACHED;
}

/*
 * Prepare MMU for usage enable it.
 */
static int mmu_init(void)
{
	struct memory_bank *bank;
	int i;

	ttb = memalign(0x10000, 0x4000);

	debug("ttb: 0x%p\n", ttb);

	/* Set the ttb register */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);

	/* Set the Domain Access Control Register */
	i = 0x3;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/* create a flat mapping using 1MiB sections */
	create_sections(0, 0, 4096, PMD_SECT_AP_WRITE | PMD_SECT_AP_READ |
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

	asm volatile (
		"bl __mmu_cache_on;"
		:
		:
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "lr", "cc", "memory"
        );

	/*
	 * Now that we have the MMU and caches on remap sdram again using
	 * page tables
	 */
	for_each_memory_bank(bank)
		arm_mmu_remap_sdram(bank);

	return 0;
}
mmu_initcall(mmu_init);

struct outer_cache_fns outer_cache;

/*
 * Clean and invalide caches, disable MMU
 */
void mmu_disable(void)
{

	if (outer_cache.disable)
		outer_cache.disable();

	asm volatile (
		"bl __mmu_cache_flush;"
		"bl __mmu_cache_off;"
		:
		:
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "lr", "cc", "memory"
	);
}

#define PAGE_ALIGN(s) ((s) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

void *dma_alloc_coherent(size_t size)
{
	void *ret;

	size = PAGE_ALIGN(size);
	ret = xmemalign(4096, size);

	dma_inv_range((unsigned long)ret, (unsigned long)ret + size);

	remap_range(ret, size, PTE_FLAGS_UNCACHED);

	return ret;
}

unsigned long virt_to_phys(void *virt)
{
	return (unsigned long)virt;
}

void *phys_to_virt(unsigned long phys)
{
	return (void *)phys;
}

void dma_free_coherent(void *mem, size_t size)
{
	remap_range(mem, size, PTE_FLAGS_CACHED);

	free(mem);
}

void dma_clean_range(unsigned long start, unsigned long end)
{
	if (outer_cache.clean_range)
		outer_cache.clean_range(start, end);
	__dma_clean_range(start, end);
}

void dma_flush_range(unsigned long start, unsigned long end)
{
	if (outer_cache.flush_range)
		outer_cache.flush_range(start, end);
	__dma_flush_range(start, end);
}

void dma_inv_range(unsigned long start, unsigned long end)
{
	if (outer_cache.inv_range)
		outer_cache.inv_range(start, end);
	__dma_inv_range(start, end);
}


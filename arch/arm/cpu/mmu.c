#include <common.h>
#include <init.h>
#include <asm/mmu.h>

static unsigned long *ttb;

void arm_create_section(unsigned long virt, unsigned long phys, int size_m,
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
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "cc", "memory"
	);
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

	ttb[virt] = (unsigned long)table | PMD_TYPE_TABLE;

	for (i = 0; i < 256; i++)
		table[i] = virt | PTE_TYPE_SMALL | PTE_SMALL_AP_UNO_SRW;

	return table;
}

/*
 * We have 8 exception vectors and the table consists of absolute
 * jumps, so we need 8 * 4 bytes for the instructions and another
 * 8 * 4 bytes for the addresses.
 */
#define ARM_VECTORS_SIZE	(sizeof(u32) * 8 * 2)

/*
 * Allocate a page, map it to the zero page and copy our exception
 * vectors there.
 */
static void vectors_init(void)
{
	u32 *exc;
	void *vectors;
	extern unsigned long exception_vectors;

	exc = arm_create_pte(0x0);

	vectors = xmemalign(PAGE_SIZE, PAGE_SIZE);
	memset(vectors, 0, PAGE_SIZE);
	memcpy(vectors, &exception_vectors, ARM_VECTORS_SIZE);

	exc[0] = (u32)vectors | PTE_TYPE_SMALL | PTE_SMALL_AP_UNO_SRW;
}

/*
 * Prepare MMU for usage and create a flat mapping. Board
 * code is responsible to remap the SDRAM cached
 */
void mmu_init(void)
{
	int i;

	ttb = memalign(0x10000, 0x4000);

	/* Set the ttb register */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);

	/* Set the Domain Access Control Register */
	i = 0x3;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/* create a flat mapping */
	arm_create_section(0, 0, 4096, PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | PMD_TYPE_SECT);

	vectors_init();
}

/*
 * enable the MMU. Should be called after mmu_init()
 */
void mmu_enable(void)
{
	asm volatile (
		"bl __mmu_cache_on;"
		:
		:
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "cc", "memory"
        );
}

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
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "cc", "memory"
	);
}

/*
 * For boards which need coherent memory for DMA. The idea
 * is simple: Setup a uncached section containing your SDRAM
 * and call setup_dma_coherent() with the offset between the
 * cached and the uncached section. dma_alloc_coherent() then
 * works using normal malloc but returns the corresponding
 * pointer in the uncached area.
 */
static unsigned long dma_coherent_offset;

void setup_dma_coherent(unsigned long offset)
{
	dma_coherent_offset = offset;
}

void *dma_alloc_coherent(size_t size)
{
	return xmemalign(4096, size) + dma_coherent_offset;
}

unsigned long virt_to_phys(void *virt)
{
	return (unsigned long)virt - dma_coherent_offset;
}

void *phys_to_virt(unsigned long phys)
{
	return (void *)(phys + dma_coherent_offset);
}

void dma_free_coherent(void *mem)
{
	free(mem - dma_coherent_offset);
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


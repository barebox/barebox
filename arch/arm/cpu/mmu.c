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
 * Prepare MMU for usage and create a flat mapping. Board
 * code is responsible to remap the SDRAM cached
 */
void mmu_init(void)
{
	int i;

	ttb = xzalloc(0x8000);
	ttb = (void *)(((unsigned long)ttb + 0x4000) & ~0x3fff);

	/* Set the ttb register */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);

	/* Set the Domain Access Control Register */
	i = 0x3;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/* create a flat mapping */
	arm_create_section(0, 0, 4096, PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | PMD_TYPE_SECT);
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


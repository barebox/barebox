#ifndef __ASM_NIOS2_DMA_MAPPING_H
#define __ASM_NIOS2_DMA_MAPPING_H

#include <common.h>
#include <xfuncs.h>

#include <asm/cache.h>

/* dma_alloc_coherent() return cache-line aligned allocation which is mapped
 * to uncached io region.
 *
 * IO_REGION_BASE should be defined in board config header file
 *   0x80000000 for nommu, 0xe0000000 for mmu
 */

#if (DCACHE_SIZE != 0)
static inline void *dma_alloc_coherent(size_t len, unsigned long *handle)
{
	void *addr = malloc(len + DCACHE_LINE_SIZE);
	if (!addr)
		return 0;
	flush_dcache_range((unsigned long)addr,(unsigned long)addr + len + DCACHE_LINE_SIZE);
	*handle = ((unsigned long)addr +
		   (DCACHE_LINE_SIZE - 1)) &
		~(DCACHE_LINE_SIZE - 1) & ~(IO_REGION_BASE);
	return (void *)(*handle | IO_REGION_BASE);
}
#else
static inline void *dma_alloc_coherent(size_t len, unsigned long *handle)
{
	void *addr = malloc(len);
	if (!addr)
		return 0;
	*handle = (unsigned long)addr;
	return (void *)(*handle | IO_REGION_BASE);
}
#endif

#if (DCACHE_SIZE != 0)
#define dma_alloc dma_alloc
static inline void *dma_alloc(size_t size)
{
	return xmemalign(DCACHE_LINE_SIZE, ALIGN(size, DCACHE_LINE_SIZE));
}
#endif

#endif /* __ASM_NIOS2_DMA_MAPPING_H */

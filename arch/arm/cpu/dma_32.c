#include <dma.h>
#include <asm/mmu.h>

void arch_sync_dma_for_device(void *vaddr, size_t size,
			      enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;
	unsigned long end = start + size;

	if (dir == DMA_FROM_DEVICE) {
		__dma_inv_range(start, end);
		if (outer_cache.inv_range)
			outer_cache.inv_range(start, end);
	} else {
		__dma_clean_range(start, end);
		if (outer_cache.clean_range)
			outer_cache.clean_range(start, end);
	}
}

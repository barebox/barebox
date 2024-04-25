#include <dma.h>
#include <asm/mmu.h>
#include <asm/cache.h>

void arch_sync_dma_for_device(void *vaddr, size_t size,
                              enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;
	unsigned long end = start + size - 1;

	if (dir == DMA_FROM_DEVICE)
		v8_inv_dcache_range(start, end);
	else
		v8_flush_dcache_range(start, end);
}

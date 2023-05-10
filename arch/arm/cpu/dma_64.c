#include <dma.h>
#include <asm/mmu.h>
#include <asm/cache.h>

void dma_sync_single_for_device(dma_addr_t address, size_t size,
                                enum dma_data_direction dir)
{
	/*
	 * FIXME: This function needs a device argument to support non 1:1 mappings
	 */

	if (dir == DMA_FROM_DEVICE)
		v8_inv_dcache_range(address, address + size - 1);
	else
		v8_flush_dcache_range(address, address + size - 1);
}

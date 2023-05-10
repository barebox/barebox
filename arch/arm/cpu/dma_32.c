#include <dma.h>
#include <asm/mmu.h>

void dma_sync_single_for_device(dma_addr_t address, size_t size,
				enum dma_data_direction dir)
{
	/*
	 * FIXME: This function needs a device argument to support non 1:1 mappings
	 */

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

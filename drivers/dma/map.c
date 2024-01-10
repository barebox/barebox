/* SPDX-License-Identifier: GPL-2.0-only */
#include <dma.h>
#include "debug.h"

void dma_sync_single_for_cpu(struct device *dev, dma_addr_t address,
			     size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	debug_dma_sync_single_for_cpu(dev, address, size, dir);

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_cpu(ptr, size, dir);
}

void dma_sync_single_for_device(struct device *dev, dma_addr_t address,
					      size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	debug_dma_sync_single_for_device(dev, address, size, dir);

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_device(ptr, size, dir);
}

dma_addr_t dma_map_single(struct device *dev, void *ptr,
					size_t size, enum dma_data_direction dir)
{
	dma_addr_t dma_addr = cpu_to_dma(dev, ptr);

	debug_dma_map(dev, ptr, size, dir, dma_addr);

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_device(ptr, size, dir);

	return dma_addr;
}

void dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
				    size_t size, enum dma_data_direction dir)
{
	if (!dev_is_dma_coherent(dev))
		dma_sync_single_for_cpu(dev, dma_addr, size, dir);

	debug_dma_unmap(dev, dma_addr, size, dir);
}

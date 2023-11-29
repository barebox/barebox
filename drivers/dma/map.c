/* SPDX-License-Identifier: GPL-2.0-only */
#include <dma.h>

void dma_sync_single_for_cpu(struct device *dev, dma_addr_t address,
			     size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	arch_sync_dma_for_cpu(ptr, size, dir);
}

void dma_sync_single_for_device(struct device *dev, dma_addr_t address,
					      size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	arch_sync_dma_for_device(ptr, size, dir);
}

dma_addr_t dma_map_single(struct device *dev, void *ptr,
					size_t size, enum dma_data_direction dir)
{
	arch_sync_dma_for_device(ptr, size, dir);

	return cpu_to_dma(dev, ptr);
}

void dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
				    size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(dev, dma_addr, size, dir);
}

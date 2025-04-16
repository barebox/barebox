/* SPDX-License-Identifier: GPL-2.0-only */
#include <dma.h>
#include <driver.h>
#include "debug.h"

void *dma_alloc(size_t size)
{
	return xmemalign(DMA_ALIGNMENT, ALIGN(size, DMA_ALIGNMENT));
}
EXPORT_SYMBOL(dma_alloc);

void *dma_zalloc(size_t size)
{
	void *buf;

	buf = dma_alloc(size);
	if (buf)
		memset(buf, 0x00, size);

	return buf;
}
EXPORT_SYMBOL(dma_zalloc);

void dma_sync_single_for_cpu(struct device *dev, dma_addr_t address,
			     size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	debug_dma_sync_single_for_cpu(dev, address, size, dir);

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_cpu(ptr, size, dir);
}
EXPORT_SYMBOL(dma_sync_single_for_cpu);

void dma_sync_single_for_device(struct device *dev, dma_addr_t address,
					      size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	debug_dma_sync_single_for_device(dev, address, size, dir);

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_device(ptr, size, dir);
}
EXPORT_SYMBOL(dma_sync_single_for_device);

dma_addr_t dma_map_single(struct device *dev, void *ptr,
					size_t size, enum dma_data_direction dir)
{
	dma_addr_t dma_addr = cpu_to_dma(dev, ptr);

	debug_dma_map(dev, ptr, size, dir, dma_addr);

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_device(ptr, size, dir);

	return dma_addr;
}
EXPORT_SYMBOL(dma_map_single);

void dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
				    size_t size, enum dma_data_direction dir)
{
	if (!dev_is_dma_coherent(dev))
		dma_sync_single_for_cpu(dev, dma_addr, size, dir);

	debug_dma_unmap(dev, dma_addr, size, dir);
}
EXPORT_SYMBOL(dma_unmap_single);

/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __ASM_DMA_H
#define __ASM_DMA_H

/* empty */
#include <io.h>

#define arch_sync_dma_for_cpu arch_sync_dma_for_cpu
static inline void arch_sync_dma_for_cpu(void *vaddr, size_t size,
					 enum dma_data_direction dir)
{
}

#define arch_sync_dma_for_device arch_sync_dma_for_device
static inline void arch_sync_dma_for_device(void *vaddr, size_t size,
					    enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_cpu(struct device *dev, dma_addr_t address,
					   size_t size, enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_device(struct device *dev, dma_addr_t address,
					      size_t size, enum dma_data_direction dir)
{
}

static inline dma_addr_t dma_map_single(struct device *dev, void *ptr,
					size_t size, enum dma_data_direction dir)
{
	return virt_to_phys(ptr);
}

static inline void dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
				    size_t size, enum dma_data_direction dir)
{
}

#endif /* __ASM_DMA_H */

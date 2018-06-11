/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#include <common.h>

#define dma_alloc dma_alloc
static inline void *dma_alloc(size_t size)
{
	return xmemalign(64, ALIGN(size, 64));
}

#ifndef CONFIG_MMU
static inline void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	void *ret = xmemalign(4096, size);
	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	return ret;
}

static inline void *dma_alloc_writecombine(size_t size, dma_addr_t *dma_handle)
{
	return dma_alloc_coherent(size, dma_handle);
}

static inline void dma_free_coherent(void *mem, dma_addr_t dma_handle,
				     size_t size)
{
	free(mem);
}

static inline dma_addr_t dma_map_single(struct device_d *dev, void *ptr, size_t size,
					enum dma_data_direction dir)
{
	return (dma_addr_t)ptr;
}

static inline void dma_unmap_single(struct device_d *dev, dma_addr_t addr, size_t size,
				    enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
					   enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_device(dma_addr_t address, size_t size,
					      enum dma_data_direction dir)
{
}
#endif

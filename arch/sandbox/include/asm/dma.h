/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __ASM_DMA_H
#define __ASM_DMA_H

#include <linux/kernel.h>
#include <linux/string.h>
#include <driver.h>

#define dma_alloc dma_alloc
static inline void *dma_alloc(size_t size)
{
	return xmemalign(64, ALIGN(size, 64));
}

#define dma_alloc_coherent dma_alloc_coherent
static inline void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	void *ret = xmemalign(4096, size);
	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	memset(ret, 0, size);

	return ret;
}

#define dma_alloc_writecombine dma_alloc_writecombine
static inline void *dma_alloc_writecombine(size_t size, dma_addr_t *dma_handle)
{
	return dma_alloc_coherent(size, dma_handle);
}

#define dma_free_coherent dma_free_coherent
static inline void dma_free_coherent(void *mem, dma_addr_t dma_handle,
				     size_t size)
{
	free(mem);
}

#define dma_sync_single_for_cpu dma_sync_single_for_cpu
static inline void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
					   enum dma_data_direction dir)
{
}

#define dma_sync_single_for_device dma_sync_single_for_device
static inline void dma_sync_single_for_device(dma_addr_t address, size_t size,
					      enum dma_data_direction dir)
{
}

#endif /* __ASM_DMA_H */

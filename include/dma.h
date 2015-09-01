/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __DMA_H
#define __DMA_H

#include <malloc.h>
#include <xfuncs.h>

#include <dma-dir.h>
#include <asm/dma.h>

#define DMA_ADDRESS_BROKEN	NULL

#ifndef dma_alloc
static inline void *dma_alloc(size_t size)
{
	return xmalloc(size);
}
#endif

#ifndef dma_free
static inline void dma_free(void *mem)
{
	free(mem);
}
#endif

/* streaming DMA - implement the below calls to support HAS_DMA */
void dma_sync_single_for_cpu(unsigned long address, size_t size,
			     enum dma_data_direction dir);

void dma_sync_single_for_device(unsigned long address, size_t size,
				enum dma_data_direction dir);

void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle);
void dma_free_coherent(void *mem, dma_addr_t dma_handle, size_t size);
void *dma_alloc_writecombine(size_t size, dma_addr_t *dma_handle);

#endif /* __DMA_H */

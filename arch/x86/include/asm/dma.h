/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 Marc Kleine-Budde <mkl@pengutronix.de> */

#ifndef __ASM_DMA_H
#define __ASM_DMA_H

#include <linux/string.h>
#include <linux/compiler.h>
#include <xfuncs.h>
#include <malloc.h>

/*
 * x86 is cache coherent, so we need not do anything special here
 */

static inline void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	void *ret = xmemalign(4096, size);
	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	memset(ret, 0, size);

	return ret;
}

static inline void dma_free_coherent(void *mem, dma_addr_t dma_handle,
				     size_t size)
{
	free(mem);
}

static inline void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
					   enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_device(dma_addr_t address, size_t size,
					      enum dma_data_direction dir)
{
}

#endif /* __ASM_DMA_H */

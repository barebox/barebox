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

static inline void dma_free_coherent(void *mem, dma_addr_t dma_handle,
				     size_t size)
{
	free(mem);
}
#endif

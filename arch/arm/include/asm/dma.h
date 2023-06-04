/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 Marc Kleine-Budde <mkl@pengutronix.de> */

#include <common.h>

#define dma_alloc dma_alloc
static inline void *dma_alloc(size_t size)
{
	return xmemalign(64, ALIGN(size, 64));
}

#ifndef CONFIG_MMU
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
#endif

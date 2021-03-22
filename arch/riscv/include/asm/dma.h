/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_DMA_MAPPING_H
#define _ASM_DMA_MAPPING_H

#include <common.h>
#include <xfuncs.h>
#include <linux/build_bug.h>
#include <malloc.h>

#ifdef CONFIG_MMU
#error DMA stubs need be replaced when using MMU and caches
#endif

static inline void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	void *ret;

	ret = xmemalign(PAGE_SIZE, size);

	memset(ret, 0, size);

	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	return ret;
}

static inline void dma_free_coherent(void *vaddr, dma_addr_t dma_handle,
				     size_t size)
{
	free(vaddr);
}

static inline void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
					   enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_device(dma_addr_t address, size_t size,
					      enum dma_data_direction dir)
{
}

#endif /* _ASM_DMA_MAPPING_H */

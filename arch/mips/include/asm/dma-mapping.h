#ifndef _ASM_DMA_MAPPING_H
#define _ASM_DMA_MAPPING_H

#include <xfuncs.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <malloc.h>

static inline void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	void *ret;

	ret = xmemalign(PAGE_SIZE, size);

	if (dma_handle)
		*dma_handle = CPHYSADDR(ret);

	return (void *)CKSEG1ADDR(ret);
}

static inline void dma_free_coherent(void *vaddr, dma_addr_t dma_handle,
				     size_t size)
{
	free(vaddr);
}

#endif /* _ASM_DMA_MAPPING_H */

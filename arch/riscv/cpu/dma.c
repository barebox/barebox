/* SPDX-License-Identifier: GPL-2.0 */

#include <common.h>
#include <xfuncs.h>
#include <asm/dma.h>
#include <malloc.h>

static void __dma_flush_range(dma_addr_t start, dma_addr_t end)
{
}

static void *__dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	void *ret;

	ret = xmemalign(PAGE_SIZE, size);

	memset(ret, 0, size);

	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	return ret;
}

static void __dma_free_coherent(void *vaddr, dma_addr_t dma_handle, size_t size)
{
	free(vaddr);
}

static const struct dma_ops coherent_dma_ops = {
	.alloc_coherent = __dma_alloc_coherent,
	.free_coherent = __dma_free_coherent,
	.flush_range = __dma_flush_range,
	.inv_range = __dma_flush_range,
};

static const struct dma_ops *dma_ops = &coherent_dma_ops;

void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	return dma_ops->alloc_coherent(size, dma_handle);
}

void dma_free_coherent(void *vaddr, dma_addr_t dma_handle, size_t size)
{
	dma_ops->free_coherent(vaddr, dma_handle, size);
}

void dma_set_ops(const struct dma_ops *ops)
{
	dma_ops = ops;
}

void arch_sync_dma_for_cpu(void *vaddr, size_t size,
			   enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;
	unsigned long end = start + size;

        if (dir != DMA_TO_DEVICE)
                dma_ops->inv_range(start, end);
}

void arch_sync_dma_for_device(void *vaddr, size_t size,
			      enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;
	unsigned long end = start + size;

        if (dir == DMA_FROM_DEVICE)
                dma_ops->inv_range(start, end);
        else
                dma_ops->flush_range(start, end);
}

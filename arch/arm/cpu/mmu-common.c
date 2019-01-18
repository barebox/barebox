
#define pr_fmt(fmt)	"mmu: " fmt

#include <common.h>
#include <init.h>
#include <dma-dir.h>
#include <dma.h>
#include <mmu.h>
#include <asm/system.h>
#include <memory.h>
#include "mmu.h"


void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
			     enum dma_data_direction dir)
{
	if (dir != DMA_TO_DEVICE)
		dma_inv_range((void *)address, size);
}

dma_addr_t dma_map_single(struct device_d *dev, void *ptr, size_t size,
			  enum dma_data_direction dir)
{
	unsigned long addr = (unsigned long)ptr;

	dma_sync_single_for_device(addr, size, dir);

	return addr;
}

void dma_unmap_single(struct device_d *dev, dma_addr_t addr, size_t size,
		      enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(addr, size, dir);
}

void *dma_alloc_map(size_t size, dma_addr_t *dma_handle, unsigned flags)
{
	void *ret;

	size = PAGE_ALIGN(size);
	ret = xmemalign(PAGE_SIZE, size);
	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	memset(ret, 0, size);
	dma_flush_range(ret, size);

	arch_remap_range(ret, size, flags);

	return ret;
}

void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	return dma_alloc_map(size, dma_handle, MAP_UNCACHED);
}

void dma_free_coherent(void *mem, dma_addr_t dma_handle, size_t size)
{
	size = PAGE_ALIGN(size);
	arch_remap_range(mem, size, MAP_CACHED);

	free(mem);
}

static int mmu_init(void)
{
	if (list_empty(&memory_banks))
		/*
		 * If you see this it means you have no memory registered.
		 * This can be done either with arm_add_mem_device() in an
		 * initcall prior to mmu_initcall or via devicetree in the
		 * memory node.
		 */
		panic("MMU: No memory bank found! Cannot continue\n");

	__mmu_init(get_cr() & CR_M);

	return 0;
}
mmu_initcall(mmu_init);
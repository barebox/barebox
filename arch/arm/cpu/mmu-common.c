// SPDX-License-Identifier: GPL-2.0-only


#define pr_fmt(fmt)	"mmu: " fmt

#include <common.h>
#include <init.h>
#include <dma-dir.h>
#include <dma.h>
#include <mmu.h>
#include <asm/system.h>
#include <asm/barebox-arm.h>
#include <memory.h>
#include <zero_page.h>
#include "mmu-common.h"

void arch_sync_dma_for_cpu(void *vaddr, size_t size,
			   enum dma_data_direction dir)
{
	if (dir != DMA_TO_DEVICE)
		dma_inv_range(vaddr, size);
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

	remap_range(ret, size, flags);

	return ret;
}

void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle)
{
	/*
	 * FIXME: This function needs a device argument to support non 1:1 mappings
	 */

	return dma_alloc_map(size, dma_handle, MAP_UNCACHED);
}

void dma_free_coherent(void *mem, dma_addr_t dma_handle, size_t size)
{
	size = PAGE_ALIGN(size);
	remap_range(mem, size, MAP_CACHED);

	free(mem);
}

void zero_page_access(void)
{
	remap_range(0x0, PAGE_SIZE, MAP_CACHED);
}

void zero_page_faulting(void)
{
	remap_range(0x0, PAGE_SIZE, MAP_FAULT);
}

static int mmu_init(void)
{
	if (list_empty(&memory_banks)) {
		resource_size_t start;
		int ret;

		/*
		 * If you see this it means you have no memory registered.
		 * This can be done either with arm_add_mem_device() in an
		 * initcall prior to mmu_initcall or via devicetree in the
		 * memory node.
		 */
		pr_emerg("No memory bank registered. Limping along with initial memory\n");

		start = arm_mem_membase_get();
		ret = barebox_add_memory_bank("initmem", start,
					      arm_mem_endmem_get() - start);
		if (ret)
			panic("");
	}

	__mmu_init(get_cr() & CR_M);

	return 0;
}
mmu_initcall(mmu_init);

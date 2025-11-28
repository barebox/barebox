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
#include <range.h>
#include "mmu-common.h"
#include <efi/efi-mode.h>

const char *map_type_tostr(maptype_t map_type)
{
	map_type &= ~ARCH_MAP_FLAG_PAGEWISE;

	switch (map_type) {
	case ARCH_MAP_CACHED_RWX:	return "RWX";
	case ARCH_MAP_CACHED_RO:	return "RO";
	case MAP_CACHED:		return "CACHED";
	case MAP_UNCACHED:		return "UNCACHED";
	case MAP_CODE:			return "CODE";
	case MAP_WRITECOMBINE:		return "WRITECOMBINE";
	case MAP_FAULT:			return "FAULT";
	default:			return "<unknown>";
	}
}

void arch_sync_dma_for_cpu(void *vaddr, size_t size,
			   enum dma_data_direction dir)
{
	if (dir != DMA_TO_DEVICE)
		dma_inv_range(vaddr, size);
}

void *dma_alloc_map(struct device *dev,
		    size_t size, dma_addr_t *dma_handle, maptype_t map_type)
{
	void *ret;

	size = PAGE_ALIGN(size);
	ret = xmemalign(PAGE_SIZE, size);
	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	memset(ret, 0, size);
	dma_flush_range(ret, size);

	remap_range(ret, size, map_type);

	return ret;
}

void *dma_alloc_coherent(struct device *dev,
			 size_t size, dma_addr_t *dma_handle)
{
	/*
	 * FIXME: This function needs a device argument to support non 1:1 mappings
	 */

	return dma_alloc_map(dev, size, dma_handle, MAP_UNCACHED);
}

void dma_free_coherent(struct device *dev,
		       void *mem, dma_addr_t dma_handle, size_t size)
{
	size = PAGE_ALIGN(size);
	remap_range(mem, size, MAP_CACHED);

	free(mem);
}

void *dma_alloc_writecombine(struct device *dev, size_t size, dma_addr_t *dma_handle)
{
	return dma_alloc_map(dev, size, dma_handle, MAP_WRITECOMBINE);
}

bool zero_page_remappable(void)
{
	return get_cr() & CR_M;
}

void zero_page_access(void)
{
	remap_range(0x0, PAGE_SIZE, MAP_CACHED);
}

void zero_page_faulting(void)
{
	remap_range(0x0, PAGE_SIZE, MAP_FAULT);
}

/**
 * remap_range_end - remap a range identified by [start, end)
 *
 * @start:    start of the range
 * @end:      end of the first range (exclusive)
 * @map_type: mapping type to apply
 */
static inline void remap_range_end(unsigned long start, unsigned long end,
				   unsigned map_type)
{
	remap_range((void *)start, end - start, map_type);
}

static inline void remap_range_end_sans_text(unsigned long start, unsigned long end,
					     unsigned map_type)
{
	unsigned long text_start = (unsigned long)&_stext;
	unsigned long text_end = (unsigned long)&_etext;

	if (region_overlap_end_exclusive(start, end, text_start, text_end)) {
		remap_range_end(start, text_start, MAP_CACHED);
		/* skip barebox segments here, will be mapped later */
		start = text_end;
	}

	remap_range_end(start, end, MAP_CACHED);
}

static void mmu_remap_memory_banks(void)
{
	struct memory_bank *bank;
	unsigned long code_start = (unsigned long)&_stext;
	unsigned long code_size = (unsigned long)&__start_rodata - (unsigned long)&_stext;
	unsigned long rodata_start = (unsigned long)&__start_rodata;
	unsigned long rodata_size = (unsigned long)&__end_rodata - rodata_start;

	/*
	 * Early mmu init will have mapped everything but the initial memory area
	 * (excluding final OPTEE_SIZE bytes) uncached. We have now discovered
	 * all memory banks, so let's map all pages, excluding reserved memory areas
	 * and barebox text area cacheable.
	 *
	 * This code will become much less complex once we switch over to using
	 * CONFIG_MEMORY_ATTRIBUTES for MMU as well.
	 */
	for_each_memory_bank(bank) {
		struct resource *rsv;
		resource_size_t pos;

		pos = bank->start;

		/* Skip reserved regions */
		for_each_reserved_region(bank, rsv) {
			if (pos != rsv->start)
				remap_range_end_sans_text(pos, rsv->start, MAP_CACHED);
			pos = rsv->end + 1;
		}

		remap_range_end_sans_text(pos, bank->start + bank->size, MAP_CACHED);
	}

	remap_range((void *)code_start, code_size, MAP_CODE);
	remap_range((void *)rodata_start, rodata_size, ARCH_MAP_CACHED_RO);

	setup_trap_pages();
}

static int mmu_init(void)
{
	if (efi_is_payload())
		return 0;

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
	mmu_remap_memory_banks();

	return 0;
}
mmu_initcall(mmu_init);

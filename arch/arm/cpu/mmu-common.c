
#define pr_fmt(fmt)	"mmu: " fmt

#include <common.h>
#include <dma-dir.h>
#include <dma.h>
#include <mmu.h>

#include "mmu.h"

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

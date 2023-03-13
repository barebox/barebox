// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2015, 2016 Peter Mamonov <pmamonov@gmail.com>
 */

#include <dma.h>
#include <asm/io.h>

static inline void __dma_sync_mips(unsigned long addr, size_t size,
				   enum dma_data_direction direction)
{
	switch (direction) {
	case DMA_TO_DEVICE:
		dma_flush_range(addr, addr + size);
		break;

	case DMA_FROM_DEVICE:
		dma_inv_range(addr, addr + size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_flush_range(addr, addr + size);
		break;

	default:
		BUG();
	}
}

void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
			     enum dma_data_direction dir)
{
	unsigned long virt = (unsigned long)phys_to_virt(address);

	__dma_sync_mips(virt, size, dir);
}

void dma_sync_single_for_device(dma_addr_t address, size_t size,
				enum dma_data_direction dir)
{
	unsigned long virt = (unsigned long)phys_to_virt(address);

	__dma_sync_mips(virt, size, dir);
}

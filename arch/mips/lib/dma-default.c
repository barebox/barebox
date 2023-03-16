// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2015, 2016 Peter Mamonov <pmamonov@gmail.com>
 */

#include <dma.h>
#include <asm/io.h>

void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
			     enum dma_data_direction dir)
{
	unsigned long virt = (unsigned long)phys_to_virt(address);

	switch (dir) {
	case DMA_TO_DEVICE:
		break;
	case DMA_FROM_DEVICE:
	case DMA_BIDIRECTIONAL:
		dma_inv_range(virt, virt + size);
		break;
	default:
		BUG();
	}
}

void dma_sync_single_for_device(dma_addr_t address, size_t size,
				enum dma_data_direction dir)
{
	unsigned long virt = (unsigned long)phys_to_virt(address);

	switch (dir) {
	case DMA_FROM_DEVICE:
		dma_inv_range(virt, virt + size);
		break;
	case DMA_TO_DEVICE:
	case DMA_BIDIRECTIONAL:
		dma_flush_range(virt, virt + size);
		break;
	default:
		BUG();
	}
}

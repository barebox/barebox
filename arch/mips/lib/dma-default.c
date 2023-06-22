// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2015, 2016 Peter Mamonov <pmamonov@gmail.com>
 */

#include <dma.h>
#include <asm/io.h>

void arch_sync_dma_for_cpu(void *vaddr, size_t size,
			   enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;

	switch (dir) {
	case DMA_TO_DEVICE:
		break;
	case DMA_FROM_DEVICE:
	case DMA_BIDIRECTIONAL:
		dma_inv_range(start, start + size);
		break;
	default:
		BUG();
	}
}

void arch_sync_dma_for_device(void *vaddr, size_t size,
			      enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;

	switch (dir) {
	case DMA_FROM_DEVICE:
		dma_inv_range(start, start + size);
		break;
	case DMA_TO_DEVICE:
	case DMA_BIDIRECTIONAL:
		dma_flush_range(start, start + size);
		break;
	default:
		BUG();
	}
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2015, 2016 Peter Mamonov <pmamonov@gmail.com>
 */

#include <dma.h>
#include <asm/io.h>

#if defined(CONFIG_CPU_MIPS32) || \
	defined(CONFIG_CPU_MIPS64)
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
#else
static inline void __dma_sync_mips(void *addr, size_t size,
	enum dma_data_direction direction)
{
}
#endif

void dma_sync_single_for_cpu(dma_addr_t address, size_t size,
			     enum dma_data_direction dir)
{
	__dma_sync_mips(address, size, dir);
}

void dma_sync_single_for_device(dma_addr_t address, size_t size,
				enum dma_data_direction dir)
{
	__dma_sync_mips(address, size, dir);
}

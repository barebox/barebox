// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2021 Yann Sionneau <ysionneau@kalray.eu>, Kalray Inc.

#include <dma.h>
#include <asm/barrier.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <asm/sfr.h>
#include <asm/sys_arch.h>

/*
 * The implementation of arch should follow the following rules:
 *		map		for_cpu		for_device	unmap
 * TO_DEV	writeback	none		writeback	none
 * FROM_DEV	invalidate	invalidate(*)	invalidate	invalidate(*)
 * BIDIR	writeback	invalidate	writeback	invalidate
 *
 * (*) - only necessary if the CPU speculatively prefetches.
 *
 * (see https://lkml.org/lkml/2018/5/18/979)
 */

void dma_sync_single_for_device(dma_addr_t addr, size_t size,
				enum dma_data_direction dir)
{
	/* dcache is Write-Through: no need to flush to force writeback */
	switch (dir) {
	case DMA_FROM_DEVICE:
		invalidate_dcache_range(addr, addr + size);
		break;
	case DMA_TO_DEVICE:
	case DMA_BIDIRECTIONAL:
		/* allow device to read buffer written by CPU */
		wmb();
		break;
	default:
		BUG();
	}
}

void dma_sync_single_for_cpu(dma_addr_t addr, size_t size,
				enum dma_data_direction dir)
{
	/* CPU does not speculatively prefetches */
	switch (dir) {
	case DMA_FROM_DEVICE:
		/* invalidate has been done during map/for_device */
	case DMA_TO_DEVICE:
		break;
	case DMA_BIDIRECTIONAL:
		invalidate_dcache_range(addr, addr + size);
		break;
	default:
		BUG();
	}
}

#define KVX_DDR_ALIAS_OFFSET \
	(KVX_DDR_64BIT_RAM_WINDOW_BA - KVX_DDR_32BIT_RAM_WINDOW_BA)
#define KVX_DDR_ALIAS_WINDOW \
	(KVX_DDR_64BIT_RAM_WINDOW_BA + KVX_DDR_ALIAS_OFFSET)

/* Local smem is aliased between 0 and 16MB */
#define KVX_SMEM_LOCAL_ALIAS 0x1000000ULL

dma_addr_t dma_map_single(struct device_d *dev, void *ptr, size_t size,
			  enum dma_data_direction dir)
{
	uintptr_t addr = (uintptr_t) ptr;

	dma_sync_single_for_device(addr, size, dir);

	/* Local smem alias should never be used for dma */
	if (addr < KVX_SMEM_LOCAL_ALIAS)
		return addr + (1 + kvx_cluster_id()) * KVX_SMEM_LOCAL_ALIAS;

	if (dev->dma_mask && addr <= dev->dma_mask)
		return addr;

	if (addr >= KVX_DDR_ALIAS_WINDOW)
		return DMA_ERROR_CODE;

	addr -= KVX_DDR_ALIAS_OFFSET;
	if (dev->dma_mask && addr > dev->dma_mask)
		return DMA_ERROR_CODE;

	return addr;
}

void dma_unmap_single(struct device_d *dev, dma_addr_t addr, size_t size,
		      enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(addr, size, dir);
}

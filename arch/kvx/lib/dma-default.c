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

void arch_sync_dma_for_device(void *vaddr, size_t size,
			      enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;

	/* dcache is Write-Through: no need to flush to force writeback */
	switch (dir) {
	case DMA_FROM_DEVICE:
		invalidate_dcache_range(start, start + size);
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

void arch_sync_dma_for_cpu(void *vaddr, size_t size,
			   enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;

	/* CPU does not speculatively prefetches */
	switch (dir) {
	case DMA_FROM_DEVICE:
		/* invalidate has been done during map/for_device */
	case DMA_TO_DEVICE:
		break;
	case DMA_BIDIRECTIONAL:
		invalidate_dcache_range(start, start + size);
		break;
	default:
		BUG();
	}
}

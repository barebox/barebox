/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 */

#ifndef __DMA_H
#define __DMA_H

#include <malloc.h>
#include <xfuncs.h>
#include <linux/align.h>

#include <dma-dir.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <device.h>

#define DMA_ADDRESS_BROKEN	((dma_addr_t *)NULL)

#ifndef DMA_ALIGNMENT
#define DMA_ALIGNMENT	32
#endif

#ifdef CONFIG_HAS_DMA
void *dma_alloc(size_t size);
void *dma_zalloc(size_t size);
#else
static inline void *dma_alloc(size_t size)
{
	return malloc(size);
}

static inline void *dma_zalloc(size_t size)
{
	return calloc(size, 1);
}
#endif

static inline void dma_free(void *mem)
{
	free(mem);
}

static inline void dma_free_sensitive(void *mem)
{
	free_sensitive(mem);
}

#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))

#define DMA_MASK_NONE	0x0ULL

static inline void dma_set_mask(struct device *dev, u64 dma_mask)
{
	dev->dma_mask = dma_mask;
}

#define DMA_ERROR_CODE  (~(dma_addr_t)0)

static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return dma_addr == DMA_ERROR_CODE ||
		(dev->dma_mask && dma_addr > dev->dma_mask);
}

static inline dma_addr_t cpu_to_dma(struct device *dev, void *cpu_addr)
{
	if (dev && dev->dma_offset)
		return (unsigned long)cpu_addr - dev->dma_offset;

	return virt_to_phys(cpu_addr);
}

static inline void *dma_to_cpu(struct device *dev, dma_addr_t addr)
{
	if (dev && dev->dma_offset)
		return (void *)(addr + dev->dma_offset);

	return phys_to_virt(addr);
}

#ifndef arch_sync_dma_for_cpu
void arch_sync_dma_for_cpu(void *vaddr, size_t size,
			   enum dma_data_direction dir);
#endif

#ifndef arch_sync_dma_for_device
void arch_sync_dma_for_device(void *vaddr, size_t size,
			      enum dma_data_direction dir);
#endif

#ifndef __PBL__
void dma_sync_single_for_cpu(struct device *dev, dma_addr_t address,
			     size_t size, enum dma_data_direction dir);

void dma_sync_single_for_device(struct device *dev, dma_addr_t address,
				size_t size, enum dma_data_direction dir);
#else
/*
 * assumes buffers are in coherent/uncached memory, e.g. because
 * MMU is only enabled in barebox_arm_entry which hasn't run yet.
 */
static inline void dma_sync_single_for_cpu(struct device *dev, dma_addr_t address,
					   size_t size, enum dma_data_direction dir)
{
	barrier_data(address);
}

static inline void dma_sync_single_for_device(struct device *dev, dma_addr_t address,
					      size_t size, enum dma_data_direction dir)
{
	barrier_data(address);
}
#endif

dma_addr_t dma_map_single(struct device *dev, void *ptr,
			  size_t size, enum dma_data_direction dir);

void dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
		      size_t size, enum dma_data_direction dir);

#ifndef dma_alloc_coherent
void *dma_alloc_coherent(size_t size, dma_addr_t *dma_handle);
#endif

#ifndef dma_free_coherent
void dma_free_coherent(void *mem, dma_addr_t dma_handle, size_t size);
#endif

#ifndef dma_alloc_writecombine
void *dma_alloc_writecombine(size_t size, dma_addr_t *dma_handle);
#endif

#endif /* __DMA_H */

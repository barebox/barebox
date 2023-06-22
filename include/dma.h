/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 */

#ifndef __DMA_H
#define __DMA_H

#include <malloc.h>
#include <xfuncs.h>
#include <linux/kernel.h>

#include <dma-dir.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <driver.h>

#define DMA_ADDRESS_BROKEN	NULL

#ifndef DMA_ALIGNMENT
#define DMA_ALIGNMENT	32
#endif

#ifndef dma_alloc
static inline void *dma_alloc(size_t size)
{
	return xmemalign(DMA_ALIGNMENT, ALIGN(size, DMA_ALIGNMENT));
}
#endif

#ifndef dma_free
static inline void dma_free(void *mem)
{
	free(mem);
}
#endif

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

#ifndef __PBL__
/* streaming DMA - implement the below calls to support HAS_DMA */
#ifndef arch_sync_dma_for_cpu
void arch_sync_dma_for_cpu(void *vaddr, size_t size,
			   enum dma_data_direction dir);
#endif

#ifndef arch_sync_dma_for_device
void arch_sync_dma_for_device(void *vaddr, size_t size,
			      enum dma_data_direction dir);
#endif
#else
#ifndef arch_sync_dma_for_cpu
/*
 * assumes buffers are in coherent/uncached memory, e.g. because
 * MMU is only enabled in barebox_arm_entry which hasn't run yet.
 */
static inline void arch_sync_dma_for_cpu(void *vaddr, size_t size,
					 enum dma_data_direction dir)
{
	barrier_data(vaddr);
}
#endif

#ifndef arch_sync_dma_for_device
static inline void arch_sync_dma_for_device(void *vaddr, size_t size,
					    enum dma_data_direction dir)
{
	barrier_data(vaddr);
}
#endif
#endif

static inline void dma_sync_single_for_cpu(struct device *dev, dma_addr_t address,
					   size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	arch_sync_dma_for_cpu(ptr, size, dir);
}

static inline void dma_sync_single_for_device(struct device *dev, dma_addr_t address,
					      size_t size, enum dma_data_direction dir)
{
	void *ptr = dma_to_cpu(dev, address);

	arch_sync_dma_for_device(ptr, size, dir);
}

static inline dma_addr_t dma_map_single(struct device *dev, void *ptr,
					size_t size, enum dma_data_direction dir)
{
	arch_sync_dma_for_device(ptr, size, dir);

	return cpu_to_dma(dev, ptr);
}

static inline void dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
				    size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(dev, dma_addr, size, dir);
}

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

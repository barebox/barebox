/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 */

#ifndef __ASM_DMA_H
#define __ASM_DMA_H

#include <linux/pagemap.h>
#include <linux/types.h>
#include <linux/minmax.h>
#include <malloc.h>
#include <xfuncs.h>
#include <asm/addrspace.h>
#include <asm/cpu-info.h>
#include <asm/io.h>
#include <asm/types.h>

#define DMA_ALIGNMENT	\
	max(current_cpu_data.dcache.linesz, current_cpu_data.scache.linesz)

struct device;

#define dma_alloc_coherent dma_alloc_coherent
static inline void *dma_alloc_coherent(struct device *dev,
				       size_t size, dma_addr_t *dma_handle)
{
	void *ptr;
	unsigned long virt;

	ptr = xmemalign(PAGE_SIZE, size);
	memset(ptr, 0, size);

	virt = (unsigned long)ptr;

	if (dma_handle)
		*dma_handle = CPHYSADDR(virt);

	dma_flush_range(virt, virt + size);

	return (void *)CKSEG1ADDR(virt);
}

#define dma_free_coherent dma_free_coherent
static inline void dma_free_coherent(struct device *dev,
				     void *vaddr, dma_addr_t dma_handle,
				     size_t size)
{
	if (IS_ENABLED(CONFIG_MMU) && vaddr)
		free((void *)CKSEG0ADDR((unsigned long)vaddr));
	else
		free(vaddr);
}

#endif /* __ASM_DMA_H */

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * Author: Joerg Roedel <joerg.roedel@amd.com>
 */

#ifndef _KERNEL_DMA_DEBUG_H
#define _KERNEL_DMA_DEBUG_H

#include <linux/types.h>

struct device;

#ifdef CONFIG_DMA_API_DEBUG
extern void debug_dma_map(struct device *dev, void *addr,
			  size_t size,
			  int direction, dma_addr_t dma_addr);

extern void debug_dma_unmap(struct device *dev, dma_addr_t addr,
			    size_t size, int direction);

extern void debug_dma_sync_single_for_cpu(struct device *dev,
					  dma_addr_t dma_handle, size_t size,
					  int direction);

extern void debug_dma_sync_single_for_device(struct device *dev,
					     dma_addr_t dma_handle,
					     size_t size, int direction);

#else /* CONFIG_DMA_API_DEBUG */
static inline void debug_dma_map(struct device *dev, void *addr,
				 size_t size,
				 int direction, dma_addr_t dma_addr)
{
}

static inline void debug_dma_unmap(struct device *dev, dma_addr_t addr,
				   size_t size, int direction)
{
}

static inline void debug_dma_sync_single_for_cpu(struct device *dev,
						 dma_addr_t dma_handle,
						 size_t size, int direction)
{
}

static inline void debug_dma_sync_single_for_device(struct device *dev,
						    dma_addr_t dma_handle,
						    size_t size, int direction)
{
}

#endif /* CONFIG_DMA_API_DEBUG */
#endif /* _KERNEL_DMA_DEBUG_H */

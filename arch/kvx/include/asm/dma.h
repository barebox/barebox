/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2021 Yann Sionneau <ysionneau@kalray.eu>, Kalray Inc. */

#ifndef __ASM_DMA_H
#define __ASM_DMA_H

#include <linux/types.h>
#include <linux/build_bug.h>
#include <malloc.h>

#define DMA_ALIGNMENT	64

struct device;

#define dma_alloc_coherent dma_alloc_coherent

/* dma_alloc_coherent is not supported as MMU support is required to map
 * uncached pages. To avoid compile-time errors for ultimately unreferenced
 * code, we declare the prototype here and let the lack of definition cause
 * a linker error if either function ends up being used.
 */
extern void *dma_alloc_coherent(struct device *dev,
			 size_t size, dma_addr_t *dma_handle);

#define dma_free_coherent dma_free_coherent
extern void dma_free_coherent(struct device *dev,
		       void *mem, dma_addr_t dma_handle,
		       size_t size);

#endif /* __ASM_DMA_H */

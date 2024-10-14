/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _RISCV_ASM_DMA_H
#define _RISCV_ASM_DMA_H

#include <linux/types.h>

struct device;

struct dma_ops {
	void *(*alloc_coherent)(struct device *dev, size_t size, dma_addr_t *dma_handle);
	void (*free_coherent)(struct device *dev, void *vaddr, dma_addr_t dma_handle, size_t size);

	void (*flush_range)(dma_addr_t start, dma_addr_t end);
	void (*inv_range)(dma_addr_t start, dma_addr_t end);
};

/* Override for SoCs with cache-incoherent DMA masters */
void dma_set_ops(const struct dma_ops *ops);

#define DMA_ALIGNMENT 64

#endif /* _ASM_DMA_MAPPING_H */

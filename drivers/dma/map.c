/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 Marc Kleine-Budde <mkl@pengutronix.de> */

#include <dma.h>

static inline dma_addr_t cpu_to_dma(struct device_d *dev, unsigned long cpu_addr)
{
	dma_addr_t dma_addr = cpu_addr;

	if (dev)
		dma_addr -= dev->dma_offset;

	return dma_addr;
}

static inline unsigned long dma_to_cpu(struct device_d *dev, dma_addr_t addr)
{
	unsigned long cpu_addr = addr;

	if (dev)
		cpu_addr += dev->dma_offset;

	return cpu_addr;
}

dma_addr_t dma_map_single(struct device_d *dev, void *ptr, size_t size,
			  enum dma_data_direction dir)
{
	unsigned long addr = (unsigned long)ptr;

	dma_sync_single_for_device(addr, size, dir);

	return cpu_to_dma(dev, addr);
}

void dma_unmap_single(struct device_d *dev, dma_addr_t dma_addr, size_t size,
		      enum dma_data_direction dir)
{
	unsigned long addr = dma_to_cpu(dev, dma_addr);

	dma_sync_single_for_cpu(addr, size, dir);
}

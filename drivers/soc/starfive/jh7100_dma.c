// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <dma.h>
#include <soc/sifive/l2_cache.h>

#define SDRAM_CACHED_BASE	0x80000000
#define SDRAM_UNCACHED_BASE	0x1000000000

static inline void *jh7100_alloc_coherent(struct device *dev,
					  size_t size, dma_addr_t *dma_handle)
{
	dma_addr_t cpu_base;
	void *ret;

	ret = xmemalign(PAGE_SIZE, size);

	memset(ret, 0, size);

	cpu_base = (dma_addr_t)ret;

	if (dma_handle)
		*dma_handle = cpu_base;

	sifive_l2_flush64_range(cpu_base, cpu_base + size);

	return ret - SDRAM_CACHED_BASE + SDRAM_UNCACHED_BASE;

}

static inline void jh7100_free_coherent(struct device *dev,
					void *vaddr, dma_addr_t dma_handle, size_t size)
{
	free((void *)dma_handle);
}

static const struct dma_ops jh7100_dma_ops = {
	.alloc_coherent = jh7100_alloc_coherent,
	.free_coherent = jh7100_free_coherent,
	.flush_range = sifive_l2_flush64_range,
	.inv_range = sifive_l2_flush64_range,
};

static int jh7100_dma_init(void)
{
	/* board drivers can claim the machine compatible, so no driver here */
	if (!of_machine_is_compatible("starfive,jh7100"))
		return 0;

	dma_set_ops(&jh7100_dma_ops);

	return 0;
}
mmu_initcall(jh7100_dma_init);

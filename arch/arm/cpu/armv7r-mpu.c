// SPDX-License-Identifier: GPL-2.0+
/*
 * Cortex-R Memory Protection Unit specific code
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - https://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */
#define pr_fmt(fmt) "armv7r-mpu: " fmt

#include <memory.h>
#include <string.h>
#include <cache.h>
#include <errno.h>
#include <tlsf.h>
#include <dma.h>
#include <linux/bitfield.h>
#include <asm/armv7r-mpu.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <asm/dma.h>
#include <asm/mmu.h>
#include <linux/printk.h>

#define MPUIR_DREGION		GENMASK(15, 8)

/**
 * Note:
 * The Memory Protection Unit(MPU) allows to partition memory into regions
 * and set individual protection attributes for each region. In absence
 * of MPU a default map[1] will take effect. make sure to run this code
 * from a region which has execution permissions by default.
 * [1] http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0460d/I1002400.html
 */

void armv7r_mpu_disable(void)
{
	u32 reg;

	reg = get_cr();
	reg &= ~CR_M;
	dsb();
	set_cr(reg);
	isb();
}

void armv7r_mpu_enable(void)
{
	u32 reg;

	reg = get_cr();
	reg |= CR_M;
	dsb();
	set_cr(reg);
	isb();
}

int armv7r_mpu_enabled(void)
{
	return get_cr() & CR_M;
}

static __maybe_unused void armv7_mpu_print_config(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		u32 addr, size, attr;

		asm volatile ("mcr p15, 0, %0, c6, c2, 0" : : "r" (i));

		asm volatile ("mrc p15, 0, %0, c6, c1, 0" : "=r" (addr));
		asm volatile ("mrc p15, 0, %0, c6, c1, 2" : "=r" (size));
		asm volatile ("mrc p15, 0, %0, c6, c1, 4" : "=r" (attr));

		pr_debug("%s: %d 0x%08x 0x%08x 0x%08x\n", __func__, i, addr, size, attr);
	}
}

void armv7r_mpu_config(struct mpu_region_config *rgn)
{
	u32 attr, val;

	pr_debug("%s: no: %d start: 0x%08x size: 0x%08x\n", __func__,
		 rgn->region_no, rgn->start_addr, rgn->reg_size);

	attr = get_attr_encoding(rgn->mr_attr);

	/* MPU Region Number Register */
	asm volatile ("mcr p15, 0, %0, c6, c2, 0" : : "r" (rgn->region_no));

	/* MPU Region Base Address Register */
	asm volatile ("mcr p15, 0, %0, c6, c1, 0" : : "r" (rgn->start_addr));

	/* MPU Region Size and Enable Register */
	if (rgn->reg_size)
		val = (rgn->reg_size << REGION_SIZE_SHIFT) | ENABLE_REGION;
	else
		val = DISABLE_REGION;
	asm volatile ("mcr p15, 0, %0, c6, c1, 2" : : "r" (val));

	/* MPU Region Access Control Register */
	val = rgn->xn << XN_SHIFT | rgn->ap << AP_SHIFT | attr;
	asm volatile ("mcr p15, 0, %0, c6, c1, 4" : : "r" (val));
}

static int armv7r_mpu_supported_regions(void)
{
	u32 num;

	asm volatile ("mrc p15, 0, %0, c0, c0, 4" : "=r" (num));

	return FIELD_GET(MPUIR_DREGION, num);
}

static int armv7r_get_unused_region(void)
{
	int i;

	for (i = 0; i < armv7r_mpu_supported_regions(); i++) {
		u32 mpu_rasr;

		/* MPU Region Number Register */
		asm volatile ("mcr p15, 0, %0, c6, c2, 0" : : "r" (i));

		asm volatile ("mrc p15, 0, %0, c6, c1, 2" : "=r" (mpu_rasr));

		if (!(mpu_rasr & ENABLE_REGION))
			return i;
	}

	return -ENOSPC;
}

int armv7r_mpu_setup_regions(struct mpu_region_config *rgns, u32 num_rgns)
{
	u32 i, supported_regions;

	supported_regions = armv7r_mpu_supported_regions();

	/* Regions to be configured cannot be greater than available regions */
	if (num_rgns > supported_regions)
		return -EINVAL;

	/**
	 * Assuming dcache might not be enabled at this point, disabling
	 * and invalidating only icache.
	 */
	icache_disable();
	icache_invalidate();

	armv7r_mpu_disable();

	for (i = 0; i < supported_regions; i++) {
		if (i < num_rgns) {
			armv7r_mpu_config(&rgns[i]);
		} else {
			struct mpu_region_config rgn = {
				.region_no = i,
			};

			armv7r_mpu_config(&rgn);
		}
	}

	armv7r_mpu_enable();

	icache_enable();

	return 0;
}

static tlsf_t dma_coherent_pool;
static unsigned long dma_coherent_start;
static unsigned long dma_coherent_size;

int armv7r_mpu_init_coherent(unsigned long start, enum size reg_size)
{
	int region_no;
	unsigned long size;
	struct mpu_region_config rgn = {
		.start_addr = start,
		.xn = XN_EN,
		.ap = PRIV_RW_USR_RW,
		.mr_attr = STRONG_ORDER,
		.reg_size = reg_size,
	};

	region_no = armv7r_get_unused_region();
	if (region_no < 0)
		return region_no;

	rgn.region_no = region_no;

	armv7r_mpu_config(&rgn);

	size = 1 << (reg_size + 1);

	dma_coherent_pool = tlsf_create_with_pool((void *)start, size);

	dma_coherent_start = start;
	dma_coherent_size = size;

	return 0;
}

static int armv7r_request_pool(void)
{
	if (dma_coherent_start && dma_coherent_size)
		request_sdram_region("DMA coherent pool",
				     dma_coherent_start, dma_coherent_size,
				     MEMTYPE_BOOT_SERVICES_DATA, MEMATTRS_RW);
	return 0;
}
postmem_initcall(armv7r_request_pool);

void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle)
{
	void *ret = tlsf_memalign(dma_coherent_pool, DMA_ALIGNMENT, size);

	if (!ret)
		return NULL;

	if (dma_handle)
		*dma_handle = (dma_addr_t)ret;

	memset(ret, 0, size);

	return ret;
}

void dma_free_coherent(struct device *dev,
		       void *mem, dma_addr_t dma_handle, size_t size)
{
	tlsf_free(dma_coherent_pool, mem);
}

void arch_sync_dma_for_cpu(void *vaddr, size_t size, enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;
	unsigned long end = start + size;

	if (dir != DMA_TO_DEVICE)
		__dma_inv_range(start, end);
}

void arch_sync_dma_for_device(void *vaddr, size_t size, enum dma_data_direction dir)
{
	unsigned long start = (unsigned long)vaddr;
	unsigned long end = start + size;

	if (dir == DMA_FROM_DEVICE)
		__dma_inv_range(start, end);
	else
		__dma_clean_range(start, end);
}

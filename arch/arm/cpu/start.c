/*
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt) "start.c: " fmt

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <of.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/secure.h>
#include <asm/unaligned.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <memory.h>
#include <uncompress.h>
#include <malloc.h>

#include <debug_ll.h>

#include "entry.h"

unsigned long arm_stack_top;
static unsigned long arm_barebox_size;
static unsigned long arm_endmem;
static void *barebox_boarddata;
static unsigned long barebox_boarddata_size;

static bool blob_is_fdt(const void *blob)
{
	return get_unaligned_be32(blob) == FDT_MAGIC;
}

static bool blob_is_compressed_fdt(const void *blob)
{
	const struct barebox_arm_boarddata_compressed_dtb *dtb = blob;

	return dtb->magic == BAREBOX_ARM_BOARDDATA_COMPRESSED_DTB_MAGIC;
}

static bool blob_is_arm_boarddata(const void *blob)
{
	const struct barebox_arm_boarddata *bd = blob;

	return bd->magic == BAREBOX_ARM_BOARDDATA_MAGIC;
}

u32 barebox_arm_machine(void)
{
	if (barebox_boarddata && blob_is_arm_boarddata(barebox_boarddata)) {
		const struct barebox_arm_boarddata *bd = barebox_boarddata;
		return bd->machine;
	} else {
		return 0;
	}
}

void *barebox_arm_boot_dtb(void)
{
	void *dtb;
	void *data;
	int ret;
	struct barebox_arm_boarddata_compressed_dtb *compressed_dtb;
	static void *boot_dtb;

	if (boot_dtb)
		return boot_dtb;

	if (barebox_boarddata && blob_is_fdt(barebox_boarddata)) {
		pr_debug("%s: using barebox_boarddata\n", __func__);
		return barebox_boarddata;
	}

	if (!IS_ENABLED(CONFIG_ARM_USE_COMPRESSED_DTB) || !barebox_boarddata
			|| !blob_is_compressed_fdt(barebox_boarddata))
		return NULL;

	compressed_dtb = barebox_boarddata;

	pr_debug("%s: using compressed_dtb\n", __func__);

	dtb = malloc(compressed_dtb->datalen_uncompressed);
	if (!dtb)
		return NULL;

	data = compressed_dtb + 1;

	ret = uncompress(data, compressed_dtb->datalen, NULL, NULL,
			dtb, NULL, NULL);
	if (ret) {
		pr_err("uncompressing dtb failed\n");
		free(dtb);
		return NULL;
	}

	boot_dtb = dtb;

	return boot_dtb;
}

static inline unsigned long arm_mem_boarddata(unsigned long membase,
					      unsigned long endmem,
					      unsigned long size)
{
	unsigned long mem;

	mem = arm_mem_barebox_image(membase, endmem, arm_barebox_size);
	mem -= ALIGN(size, 64);

	return mem;
}

unsigned long arm_mem_ramoops_get(void)
{
	return arm_mem_ramoops(0, arm_stack_top);
}
EXPORT_SYMBOL_GPL(arm_mem_ramoops_get);

unsigned long arm_mem_endmem_get(void)
{
	return arm_endmem;
}
EXPORT_SYMBOL_GPL(arm_mem_endmem_get);

static int barebox_memory_areas_init(void)
{
	if(barebox_boarddata)
		request_sdram_region("board data", (unsigned long)barebox_boarddata,
				     barebox_boarddata_size);

	return 0;
}
device_initcall(barebox_memory_areas_init);

__noreturn void barebox_non_pbl_start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	unsigned long endmem = membase + memsize;
	unsigned long malloc_start, malloc_end;
	unsigned long barebox_size = barebox_image_size + MAX_BSS_SIZE;
	unsigned long barebox_base = arm_mem_barebox_image(membase,
							   endmem,
							   barebox_size);

	if (IS_ENABLED(CONFIG_CPU_V7))
		armv7_hyp_install();

	if (IS_ENABLED(CONFIG_RELOCATABLE))
		relocate_to_adr(barebox_base);

	setup_c();

	barrier();

	pbl_barebox_break();

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	arm_endmem = endmem;
	arm_stack_top = arm_mem_stack_top(membase, endmem);
	arm_barebox_size = barebox_size;
	malloc_end = barebox_base;

	if (IS_ENABLED(CONFIG_MMU_EARLY)) {
		unsigned long ttb = arm_mem_ttb(membase, endmem);

		if (IS_ENABLED(CONFIG_PBL_IMAGE)) {
			arm_set_cache_functions();
		} else {
			pr_debug("enabling MMU, ttb @ 0x%08lx\n", ttb);
			arm_early_mmu_cache_invalidate();
			mmu_early_enable(membase, memsize, ttb);
		}
	}

	if (boarddata) {
		uint32_t totalsize = 0;
		const char *name;

		if ((unsigned long)boarddata < 8192) {
			struct barebox_arm_boarddata *bd;
			uint32_t machine_type = (unsigned long)boarddata;
			unsigned long mem = arm_mem_boarddata(membase, endmem,
							      sizeof(*bd));
			pr_debug("found machine type %d in boarddata\n",
				 machine_type);
			bd = barebox_boarddata = (void *)mem;
			barebox_boarddata_size = sizeof(*bd);
			bd->magic = BAREBOX_ARM_BOARDDATA_MAGIC;
			bd->machine = machine_type;
			malloc_end = mem;
		} else if (blob_is_fdt(boarddata)) {
			totalsize = get_unaligned_be32(boarddata + 4);
			name = "DTB";
		} else if (blob_is_compressed_fdt(boarddata)) {
			struct barebox_arm_boarddata_compressed_dtb *bd = boarddata;
			totalsize = bd->datalen + sizeof(*bd);
			name = "Compressed DTB";
		} else if (blob_is_arm_boarddata(boarddata)) {
			totalsize = sizeof(struct barebox_arm_boarddata);
			name = "machine type";
		}

		if (totalsize) {
			unsigned long mem = arm_mem_boarddata(membase, endmem,
							      totalsize);
			pr_debug("found %s in boarddata, copying to 0x%08lx\n",
				 name, mem);
			barebox_boarddata = memcpy((void *)mem, boarddata,
						   totalsize);
			barebox_boarddata_size = totalsize;
			malloc_end = mem;
		}
	}

	/*
	 * Maximum malloc space is the Kconfig value if given
	 * or 1GB.
	 */
	if (MALLOC_SIZE > 0) {
		malloc_start = malloc_end - MALLOC_SIZE;
		if (malloc_start < membase)
			malloc_start = membase;
	} else {
		malloc_start = malloc_end - (malloc_end - membase) / 2;
		if (malloc_end - malloc_start > SZ_1G)
			malloc_start = malloc_end - SZ_1G;
	}

	pr_debug("initializing malloc pool at 0x%08lx (size 0x%08lx)\n",
			malloc_start, malloc_end - malloc_start);

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);

	if (IS_ENABLED(CONFIG_BOOTM_OPTEE))
		of_add_reserve_entry(endmem - OPTEE_SIZE, endmem - 1);

	pr_debug("starting barebox...\n");

	start_barebox();
}

#ifndef CONFIG_PBL_IMAGE

void start(void);

void NAKED __section(.text_entry) start(void)
{
	barebox_arm_head();
}

#else

void start(unsigned long membase, unsigned long memsize, void *boarddata);
/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
void NAKED __section(.text_entry) start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	barebox_non_pbl_start(membase, memsize, boarddata);
}
#endif

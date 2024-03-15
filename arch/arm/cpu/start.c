// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#define pr_fmt(fmt) "start.c: " fmt

#ifdef CONFIG_DEBUG_INITCALLS
#define DEBUG
#endif

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
#include <linux/kasan.h>
#include <memory.h>
#include <uncompress.h>
#include <compressed-dtb.h>
#include <malloc.h>

#include <debug_ll.h>

#include "entry.h"

unsigned long arm_stack_top;
static unsigned long arm_barebox_size;
static unsigned long arm_endmem;
static unsigned long arm_membase;
static void *barebox_boarddata;
static unsigned long barebox_boarddata_size;

static bool blob_is_arm_boarddata(const void *blob)
{
	const struct barebox_arm_boarddata *bd = blob;

	return bd->magic == BAREBOX_ARM_BOARDDATA_MAGIC;
}

const struct barebox_boarddata *barebox_get_boarddata(void)
{
	if (!barebox_boarddata || !blob_is_arm_boarddata(barebox_boarddata))
		return NULL;

	return barebox_boarddata;
}

u32 barebox_arm_machine(void)
{
	const struct barebox_boarddata *bd = barebox_get_boarddata();
	return bd ? bd->machine : 0;
}

void *barebox_arm_boot_dtb(void)
{
	void *dtb;
	int ret = 0;
	struct barebox_boarddata_compressed_dtb *compressed_dtb;
	static void *boot_dtb;

	if (boot_dtb)
		return boot_dtb;

	if (barebox_boarddata && blob_is_fdt(barebox_boarddata)) {
		pr_debug("%s: using barebox_boarddata\n", __func__);
		return barebox_boarddata;
	}

	if (!fdt_blob_can_be_decompressed(barebox_boarddata))
		return NULL;

	compressed_dtb = barebox_boarddata;

	pr_debug("%s: using compressed_dtb\n", __func__);

	dtb = malloc(compressed_dtb->datalen_uncompressed);
	if (!dtb)
		return NULL;

	if (IS_ENABLED(CONFIG_IMAGE_COMPRESSION_NONE))
		memcpy(dtb, compressed_dtb->data,
		       compressed_dtb->datalen_uncompressed);
	else
		ret = uncompress(compressed_dtb->data, compressed_dtb->datalen,
				 NULL, NULL, dtb, NULL, NULL);

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
	return arm_mem_ramoops(arm_stack_top);
}
EXPORT_SYMBOL_GPL(arm_mem_ramoops_get);

unsigned long arm_mem_endmem_get(void)
{
	return arm_endmem;
}
EXPORT_SYMBOL_GPL(arm_mem_endmem_get);

unsigned long arm_mem_membase_get(void)
{
	return arm_membase;
}
EXPORT_SYMBOL_GPL(arm_mem_membase_get);

static int barebox_memory_areas_init(void)
{
	if(barebox_boarddata)
		request_sdram_region("board data", (unsigned long)barebox_boarddata,
				     barebox_boarddata_size);

	if (IS_ENABLED(CONFIG_KASAN))
		request_sdram_region("kasan shadow", kasan_shadow_base,
				     mem_malloc_start() - kasan_shadow_base);

	return 0;
}
device_initcall(barebox_memory_areas_init);

__noreturn __prereloc void barebox_non_pbl_start(unsigned long membase,
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

	arm_membase = membase;
	arm_endmem = endmem;
	arm_stack_top = arm_mem_stack_top(endmem);
	arm_barebox_size = barebox_size;
	malloc_end = barebox_base;

	if (boarddata) {
		uint32_t totalsize = 0;
		const char *name;

		if (blob_is_fdt(boarddata)) {
			totalsize = get_unaligned_be32(boarddata + 4);
			name = "DTB";
		} else if (blob_is_compressed_fdt(boarddata)) {
			struct barebox_boarddata_compressed_dtb *bd = boarddata;
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

	kasan_init(membase, memsize, malloc_start - (memsize >> KASAN_SHADOW_SCALE_SHIFT));

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);

	if (IS_ENABLED(CONFIG_MMU) && !IS_ENABLED(CONFIG_PBL_IMAGE)) {
		arm_early_mmu_cache_invalidate();
		mmu_early_enable(membase, memsize);
	}

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
void NAKED __prereloc __section(.text_entry) start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	barebox_non_pbl_start(membase, memsize, boarddata);
}
#endif

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
#include <pbl/handoff-data.h>
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

const struct barebox_boarddata *barebox_get_boarddata(void)
{
	size_t size;

	return handoff_data_get_entry(HANDOFF_DATA_BOARDDATA, &size);
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
	void *blob;
	size_t size;

	if (boot_dtb)
		return boot_dtb;

	blob = handoff_data_get_entry(HANDOFF_DATA_INTERNAL_DT, &size);
	if (blob)
		return blob;

	blob = handoff_data_get_entry(HANDOFF_DATA_INTERNAL_DT_Z, &size);
	if (!blob)
		return NULL;

	if (!fdt_blob_can_be_decompressed(blob))
		return NULL;

	compressed_dtb = blob;

	pr_debug("%s: using compressed_dtb\n", __func__);

	dtb = malloc(ALIGN(compressed_dtb->datalen_uncompressed, 4));
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
		request_barebox_region("board data", (unsigned long)barebox_boarddata,
				     barebox_boarddata_size);

	if (IS_ENABLED(CONFIG_KASAN))
		request_sdram_region("kasan shadow", kasan_shadow_base,
				     mem_malloc_start() - kasan_shadow_base);

	return 0;
}
device_initcall(barebox_memory_areas_init);

__noreturn __prereloc void barebox_non_pbl_start(unsigned long membase,
		unsigned long memsize, struct handoff_data *hd)
{
	unsigned long endmem = membase + memsize;
	unsigned long malloc_start, malloc_end;
	unsigned long barebox_base = arm_mem_barebox_image(membase, endmem,
							   barebox_image_size,
							   hd);

	if (IS_ENABLED(CONFIG_CPU_V7))
		armv7_hyp_install();

	relocate_to_adr(barebox_base);

	setup_c();

	barrier();

	pbl_barebox_break();

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	arm_membase = membase;
	arm_endmem = endmem;
	arm_stack_top = arm_mem_stack_top(endmem);
	arm_barebox_size = barebox_image_size + MAX_BSS_SIZE;
	malloc_end = barebox_base;

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

	register_barebox_area(barebox_base, endmem - barebox_base);

	kasan_init(membase, memsize, malloc_start - (memsize >> KASAN_SHADOW_SCALE_SHIFT));

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);

	handoff_data_set(hd);

	if (IS_ENABLED(CONFIG_BOOTM_OPTEE))
		of_add_reserve_entry(endmem - OPTEE_SIZE, endmem - 1);

	pr_debug("starting barebox...\n");

	start_barebox();
}

void start(unsigned long membase, unsigned long memsize, struct handoff_data *hd);
/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
void NAKED __prereloc __section(.text_entry) start(unsigned long membase,
		unsigned long memsize, struct handoff_data *hd)
{
	barebox_non_pbl_start(membase, memsize, hd);
}

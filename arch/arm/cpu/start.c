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
#include <asm/unaligned.h>
#include <asm/cache.h>
#include <memory.h>

#include <debug_ll.h>
#include "mmu-early.h"

unsigned long arm_stack_top;
static void *barebox_boarddata;

static bool blob_is_fdt(const void *blob)
{
	return get_unaligned_be32(blob) == FDT_MAGIC;
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
	if (barebox_boarddata && blob_is_fdt(barebox_boarddata))
		return barebox_boarddata;
	else
		return NULL;
}

__noreturn void barebox_non_pbl_start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	unsigned long endmem = membase + memsize;
	unsigned long malloc_start, malloc_end;

	if (IS_ENABLED(CONFIG_RELOCATABLE)) {
		unsigned long barebox_base = arm_barebox_image_place(endmem);
		relocate_to_adr(barebox_base);
	}

	setup_c();

	barrier();

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	arm_stack_top = endmem;
	endmem -= STACK_SIZE; /* Stack */

	if (IS_ENABLED(CONFIG_MMU_EARLY)) {

		endmem &= ~0x3fff;
		endmem -= SZ_16K; /* ttb */

		if (IS_ENABLED(CONFIG_PBL_IMAGE)) {
			arm_set_cache_functions();
		} else {
			pr_debug("enabling MMU, ttb @ 0x%08lx\n", endmem);
			arm_early_mmu_cache_invalidate();
			mmu_early_enable(membase, memsize, endmem);
		}
	}

	if (boarddata) {
		uint32_t totalsize = 0;
		const char *name;

		if (blob_is_fdt(boarddata)) {
			totalsize = get_unaligned_be32(boarddata + 4);
			name = "DTB";
		} else if (blob_is_arm_boarddata(boarddata)) {
			totalsize = sizeof(struct barebox_arm_boarddata);
			name = "machine type";
		}

		if (totalsize) {
			endmem -= ALIGN(totalsize, 64);
			pr_debug("found %s in boarddata, copying to 0x%lu\n",
				 name, endmem);
			barebox_boarddata = memcpy((void *)endmem,
						      boarddata, totalsize);
		}
	}

	if ((unsigned long)_text > membase + memsize ||
			(unsigned long)_text < membase)
		/*
		 * barebox is either outside SDRAM or in another
		 * memory bank, so we can use the whole bank for
		 * malloc.
		 */
		malloc_end = endmem;
	else
		malloc_end = (unsigned long)_text;

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

	pr_debug("starting barebox...\n");

	start_barebox();
}

#ifndef CONFIG_PBL_IMAGE

void __naked __section(.text_entry) start(void)
{
	barebox_arm_head();
}

#else
/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
void __naked __section(.text_entry) start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	barebox_non_pbl_start(membase, memsize, boarddata);
}
#endif

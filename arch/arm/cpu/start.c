/*
 * start-arm.c
 *
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

#include <common.h>
#include <init.h>
#include <sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/unaligned.h>
#include <asm/cache.h>
#include <memory.h>

#include "mmu-early.h"

unsigned long arm_stack_top;
static unsigned long barebox_boarddata;

/*
 * return the boarddata variable passed to barebox_arm_entry
 */
unsigned long barebox_arm_boarddata(void)
{
	return barebox_boarddata;
}

static void *barebox_boot_dtb;

void *barebox_arm_boot_dtb(void)
{
	return barebox_boot_dtb;
}

static noinline __noreturn void __start(uint32_t membase, uint32_t memsize,
		uint32_t boarddata)
{
	unsigned long endmem = membase + memsize;
	unsigned long malloc_start, malloc_end;

	if (IS_ENABLED(CONFIG_RELOCATABLE)) {
		unsigned long barebox_base = arm_barebox_image_place(endmem);
		relocate_to_adr(barebox_base);
	}

	setup_c();

	barebox_boarddata = boarddata;
	arm_stack_top = endmem;
	endmem -= STACK_SIZE; /* Stack */

	if (IS_ENABLED(CONFIG_MMU_EARLY)) {

		endmem &= ~0x3fff;
		endmem -= SZ_16K; /* ttb */

		if (!IS_ENABLED(CONFIG_PBL_IMAGE)) {
			arm_early_mmu_cache_invalidate();
			mmu_early_enable(membase, memsize, endmem);
		}
	}

	/*
	 * If boarddata is a pointer inside valid memory and contains a
	 * FDT magic then use it as later to probe devices
	 */
	if (boarddata > membase && boarddata < membase + memsize &&
			get_unaligned_be32((void *)boarddata) == FDT_MAGIC) {
		uint32_t totalsize = get_unaligned_be32((void *)boarddata + 4);
		endmem -= ALIGN(totalsize, 64);
		barebox_boot_dtb = (void *)endmem;
		memcpy(barebox_boot_dtb, (void *)boarddata, totalsize);
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
	 * or 64MB.
	 */
	if (MALLOC_SIZE > 0) {
		malloc_start = malloc_end - MALLOC_SIZE;
		if (malloc_start < membase)
			malloc_start = membase;
	} else {
		malloc_start = malloc_end - (malloc_end - membase) / 2;
		if (malloc_end - malloc_start > SZ_64M)
			malloc_start = malloc_end - SZ_64M;
	}

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);

	start_barebox();
}

#ifndef CONFIG_PBL_IMAGE

void __naked __section(.text_entry) start(void)
{
	barebox_arm_head();
}

/*
 * Main ARM entry point in the uncompressed image. Call this with the memory
 * region you can spare for barebox. This doesn't necessarily have to be the
 * full SDRAM. The currently running binary can be inside or outside of this
 * region. TEXT_BASE can be inside or outside of this region. boarddata will
 * be preserved and can be accessed later with barebox_arm_boarddata().
 *
 * -> membase + memsize
 *   STACK_SIZE              - stack
 *   16KiB, aligned to 16KiB - First level page table if early MMU support
 *                             is enabled
 * -> maximum end of barebox binary
 *
 * Usually a TEXT_BASE of 1MiB below your lowest possible end of memory should
 * be fine.
 */
void __naked __noreturn barebox_arm_entry(uint32_t membase, uint32_t memsize,
                uint32_t boarddata)
{
	arm_setup_stack(membase + memsize - 16);

	arm_early_mmu_cache_invalidate();

	__start(membase, memsize, boarddata);
}
#else
/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
void __naked __section(.text_entry) start(uint32_t membase, uint32_t memsize,
                uint32_t boarddata)
{
	__start(membase, memsize, boarddata);
}
#endif

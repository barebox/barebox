/*
 * uncompress.c - uncompressor code for self extracing pbl image
 *
 * Copyright (c) 2010-2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <pbl.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/pgtable.h>
#include <asm/cache.h>

#include <debug_ll.h>

#include "mmu-early.h"

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

static int __attribute__((__used__))
	__attribute__((__section__(".image_end")))
	__image_end_dummy = 0xdeadbeef;

static void __noreturn noinline uncompress_start_payload(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	uint32_t pg_len;
	void __noreturn (*barebox)(unsigned long, unsigned long, void *);
	uint32_t endmem = membase + memsize;
	unsigned long barebox_base;
	uint32_t *image_end;
	void *pg_start;
	unsigned long pc = get_pc();

	arm_early_mmu_cache_invalidate();

	endmem -= STACK_SIZE; /* stack */

	image_end = (void *)ld_var(__image_end) - get_runtime_offset();

	if (IS_ENABLED(CONFIG_PBL_RELOCATABLE)) {
		/*
		 * If we run from inside the memory just relocate the binary
		 * to the current address. Otherwise it may be a readonly location.
		 * Copy and relocate to the start of the memory in this case.
		 */
		if (pc > membase && pc < membase + memsize)
			relocate_to_current_adr();
		else
			relocate_to_adr(membase);
	}

	if (IS_ENABLED(CONFIG_RELOCATABLE))
		barebox_base = arm_barebox_image_place(membase + memsize);
	else
		barebox_base = TEXT_BASE;

	setup_c();

	if (IS_ENABLED(CONFIG_MMU_EARLY)) {
		endmem &= ~0x3fff;
		endmem -= SZ_16K; /* ttb */
		mmu_early_enable(membase, memsize, endmem);
	}

	endmem -= SZ_128K; /* early malloc */
	free_mem_ptr = endmem;
	free_mem_end_ptr = free_mem_ptr + SZ_128K;

	/*
	 * image_end is the first location after the executable. It contains
	 * the size of the appended compressed binary followed by the binary.
	 */
	pg_start = image_end + 1;
	pg_len = *(image_end);

	pbl_barebox_uncompress((void*)barebox_base, pg_start, pg_len);

	arm_early_mmu_cache_flush();
	flush_icache();

	if (IS_ENABLED(CONFIG_THUMB2_BAREBOX))
		barebox = (void *)(barebox_base + 1);
	else
		barebox = (void *)barebox_base;

	barebox(membase, memsize, boarddata);
}

/*
 * For the multi images startup process board code jumps here. We will uncompress
 * the attached barebox image and start it.
 */
void __naked __noreturn barebox_arm_entry(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	arm_setup_stack(membase + memsize - 16);

	uncompress_start_payload(membase, memsize, boarddata);
}

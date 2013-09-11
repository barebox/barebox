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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static void noinline uncompress(uint32_t membase,
		uint32_t memsize, uint32_t boarddata)
{
	uint32_t offset;
	uint32_t pg_len;
	void __noreturn (*barebox)(uint32_t, uint32_t, uint32_t);
	uint32_t endmem = membase + memsize;
	unsigned long barebox_base;
	uint32_t *ptr;
	void *pg_start;

	arm_early_mmu_cache_invalidate();

	endmem -= STACK_SIZE; /* stack */

	if (IS_ENABLED(CONFIG_PBL_RELOCATABLE))
		relocate_to_current_adr();

	/* Get offset between linked address and runtime address */
	offset = get_runtime_offset();

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

	ptr = (void *)__image_end;
	pg_start = ptr + 1;
	pg_len = *(ptr);

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
 * Generic second stage pbl uncompressor entry
 */
ENTRY_FUNCTION(start_uncompress)(uint32_t membase, uint32_t memsize,
		uint32_t boarddata)
{
	arm_setup_stack(membase + memsize - 16);

	uncompress(membase, memsize, boarddata);
}

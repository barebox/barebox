/*
 * start-pbl.c
 *
 * Copyright (c) 2010-2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#include "mmu-early.h"

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

/*
 * First instructions in the pbl image
 */
void __naked __section(.text_head_entry) pbl_start(void)
{
	barebox_arm_head();
}

extern void *input_data;
extern void *input_data_end;

static noinline __noreturn void __barebox_arm_entry(uint32_t membase,
		uint32_t memsize, uint32_t boarddata)
{
	uint32_t offset;
	uint32_t pg_start, pg_end, pg_len;
	void __noreturn (*barebox)(uint32_t, uint32_t, uint32_t);
	uint32_t endmem = membase + memsize;
	unsigned long barebox_base;

	endmem -= STACK_SIZE; /* stack */

	arm_early_mmu_cache_invalidate();

	if (IS_ENABLED(CONFIG_PBL_RELOCATABLE))
		relocate_to_current_adr();

	/* Get offset between linked address and runtime address */
	offset = get_runtime_offset();

	pg_start = (uint32_t)&input_data - offset;
	pg_end = (uint32_t)&input_data_end - offset;
	pg_len = pg_end - pg_start;

	if (IS_ENABLED(CONFIG_RELOCATABLE))
		barebox_base = arm_barebox_image_place(membase + memsize);
	else
		barebox_base = TEXT_BASE;

	if (offset && (IS_ENABLED(CONFIG_PBL_FORCE_PIGGYDATA_COPY) ||
				region_overlap(pg_start, pg_len, barebox_base, pg_len * 4))) {
		/*
		 * copy piggydata binary to its link address
		 */
		memcpy(&input_data, (void *)pg_start, pg_len);
		pg_start = (uint32_t)&input_data;
	}

	setup_c();

	if (IS_ENABLED(CONFIG_MMU_EARLY)) {
		endmem &= ~0x3fff;
		endmem -= SZ_16K; /* ttb */
		mmu_early_enable(membase, memsize, endmem);
	}

	endmem -= SZ_128K; /* early malloc */
	free_mem_ptr = endmem;
	free_mem_end_ptr = free_mem_ptr + SZ_128K;

	pbl_barebox_uncompress((void*)barebox_base, (void *)pg_start, pg_len);

	arm_early_mmu_cache_flush();
	flush_icache();

	if (IS_ENABLED(CONFIG_THUMB2_BAREBOX))
		barebox = (void *)(barebox_base + 1);
	else
		barebox = (void *)barebox_base;

	barebox(membase, memsize, boarddata);
}

/*
 * Main ARM entry point in the compressed image. Call this with the memory
 * region you can spare for barebox. This doesn't necessarily have to be the
 * full SDRAM. The currently running binary can be inside or outside of this
 * region. TEXT_BASE can be inside or outside of this region. boarddata will
 * be preserved and can be accessed later with barebox_arm_boarddata().
 *
 * -> membase + memsize
 *   STACK_SIZE              - stack
 *   16KiB, aligned to 16KiB - First level page table if early MMU support
 *                             is enabled
 *   128KiB                  - early memory space
 * -> maximum end of barebox binary
 *
 * Usually a TEXT_BASE of 1MiB below your lowest possible end of memory should
 * be fine.
 */
void __naked __noreturn barebox_arm_entry(uint32_t membase, uint32_t memsize,
		uint32_t boarddata)
{
	arm_setup_stack(membase + memsize - 16);

	__barebox_arm_entry(membase, memsize, boarddata);
}

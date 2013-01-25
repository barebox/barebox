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

#include "mmu.h"

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

static unsigned long *ttb;

static void create_sections(unsigned long addr, int size_m, unsigned int flags)
{
	int i;

	addr >>= 20;

	for (i = size_m; i > 0; i--, addr++)
		ttb[addr] = (addr << 20) | flags;
}

static void map_cachable(unsigned long start, unsigned long size)
{
	start &= ~(SZ_1M - 1);
	size = (size + (SZ_1M - 1)) & ~(SZ_1M - 1);

	create_sections(start, size >> 20, PMD_SECT_AP_WRITE |
			PMD_SECT_AP_READ | PMD_TYPE_SECT | PMD_SECT_WB);
}

static void mmu_enable(unsigned long compressed_start, unsigned int len)
{
	int i;

	/* Set the ttb register */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);

	/* Set the Domain Access Control Register */
	i = 0x3;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	create_sections(0, 4096, PMD_SECT_AP_WRITE |
			PMD_SECT_AP_READ | PMD_TYPE_SECT);
	/*
	 * Setup all regions we need cacheable, namely:
	 * - the stack
	 * - the decompressor code
	 * - the compressed image
	 * - the uncompressed image
	 * - the early malloc space
	 */
	map_cachable(STACK_BASE, STACK_SIZE);
	map_cachable((unsigned long)&_text,
			(unsigned long)&_end - (unsigned long)&_text);
	map_cachable((unsigned long)compressed_start, len);
	map_cachable(TEXT_BASE, len * 4);
	map_cachable(free_mem_ptr, free_mem_end_ptr - free_mem_ptr);

	__mmu_cache_on();
}

static void mmu_disable(void)
{
	__mmu_cache_flush();
	__mmu_cache_off();
}

static void barebox_uncompress(void *compressed_start, unsigned int len)
{
	/*
	 * remap_cached currently does not work rendering the feature
	 * of enabling the MMU in the PBL useless. disable for now.
	 */
	int use_mmu = 0;

	/* set 128 KiB at the end of the MALLOC_BASE for early malloc */
	free_mem_ptr = MALLOC_BASE + MALLOC_SIZE - SZ_128K;
	free_mem_end_ptr = free_mem_ptr + SZ_128K;

	ttb = (void *)((free_mem_ptr - 0x4000) & ~0x3fff);

	if (use_mmu)
		mmu_enable((unsigned long)compressed_start, len);

	pbl_barebox_uncompress((void*)TEXT_BASE, compressed_start, len);

	if (use_mmu)
		mmu_disable();

	flush_icache();
}

static noinline __noreturn void __barebox_arm_entry(uint32_t membase,
		uint32_t memsize, uint32_t boarddata)
{
	uint32_t offset;
	uint32_t pg_start, pg_end, pg_len;
	void __noreturn (*barebox)(uint32_t, uint32_t, uint32_t);

	/* Get offset between linked address and runtime address */
	offset = get_runtime_offset();

	pg_start = (uint32_t)&input_data - offset;
	pg_end = (uint32_t)&input_data_end - offset;
	pg_len = pg_end - pg_start;

	if (offset && (IS_ENABLED(CONFIG_PBL_FORCE_PIGGYDATA_COPY) ||
				region_overlap(pg_start, pg_len, TEXT_BASE, pg_len * 4))) {
		/*
		 * copy piggydata binary to its link address
		 */
		memcpy(&input_data, (void *)pg_start, pg_len);
		pg_start = (uint32_t)&input_data;
	}

	setup_c();

	barebox_uncompress((void *)pg_start, pg_len);

	if (IS_ENABLED(CONFIG_THUMB2_BAREBOX))
		barebox = (void *)(TEXT_BASE + 1);
	else
		barebox = (void *)TEXT_BASE;

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

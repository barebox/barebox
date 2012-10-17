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

/*
 * The actual reset vector. This code is position independent and usually
 * does not run at the address it's linked at.
 */
#ifndef CONFIG_MACH_DO_LOWLEVEL_INIT
void __naked __bare_init reset(void)
{
	common_reset();
	board_init_lowlevel_return();
}
#endif

extern void *input_data;
extern void *input_data_end;

#define STATIC static

#ifdef CONFIG_IMAGE_COMPRESSION_LZO
#include "../../../lib/decompress_unlzo.c"
#endif

#ifdef CONFIG_IMAGE_COMPRESSION_GZIP
#include "../../../../lib/decompress_inflate.c"
#endif

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

void noinline errorfn(char *error)
{
	while (1);
}

static void barebox_uncompress(void *compressed_start, unsigned int len)
{
	void (*barebox)(void);
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

	if (IS_ENABLED(CONFIG_THUMB2_BAREBOX))
		barebox = (void *)(TEXT_BASE + 1);
	else
		barebox = (void *)TEXT_BASE;

	decompress((void *)compressed_start,
			len,
			NULL, NULL,
			(void *)TEXT_BASE, NULL, errorfn);

	if (use_mmu)
		mmu_disable();

	flush_icache();

	barebox();
}

/*
 * Board code can jump here by either returning from board_init_lowlevel
 * or by calling this function directly.
 */
void __naked board_init_lowlevel_return(void)
{
	uint32_t offset;
	uint32_t pg_start, pg_end, pg_len;

	/* Setup the stack */
	arm_setup_stack(STACK_BASE + STACK_SIZE - 16);

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
}

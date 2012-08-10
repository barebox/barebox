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

#include "mmu.h"

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

void __naked __section(.text_head_entry) pbl_start(void)
{
	barebox_arm_head();
}

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

static void create_sections(unsigned long addr, int size, unsigned int flags)
{
	int i;

	addr >>= 20;
	size >>= 20;

	for (i = size; i > 0; i--, addr++)
		ttb[addr] = (addr << 20) | flags;
}

static void map_cachable(unsigned long start, unsigned long size)
{
	start &= ~(SZ_1M - 1);
	size = (size + (SZ_1M - 1)) & ~(SZ_1M - 1);

	create_sections(start, size, PMD_SECT_AP_WRITE |
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
	void (*barebox)(void);
	int use_mmu = IS_ENABLED(CONFIG_MMU);

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
			(void *)TEXT_BASE, NULL, NULL);

	if (use_mmu)
		mmu_disable();

	/* flush I-cache before jumping to the uncompressed binary */
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));

	barebox();
}

/*
 * Board code can jump here by either returning from board_init_lowlevel
 * or by calling this function directly.
 */
void __naked __section(.text_ll_return) board_init_lowlevel_return(void)
{
	uint32_t r, addr, offset;
	uint32_t pg_start, pg_end, pg_len;

	/*
	 * Get runtime address of this function. Do not
	 * put any code above this.
	 */
	__asm__ __volatile__("1: adr %0, 1b":"=r"(addr));

	/* Setup the stack */
	r = STACK_BASE + STACK_SIZE - 16;
	__asm__ __volatile__("mov sp, %0" : : "r"(r));

	/* Get offset between linked address and runtime address */
	offset = (uint32_t)__ll_return - addr;

	pg_start = (uint32_t)&input_data - offset;
	pg_end = (uint32_t)&input_data_end - offset;
	pg_len = pg_end - pg_start;

	if (IS_ENABLED(CONFIG_PBL_FORCE_PIGGYDATA_COPY))
		goto copy_piggy_link;

	/*
	 * Check if the piggydata binary will be overwritten
	 * by the uncompressed binary or by the pbl relocation
	 */
	if (!offset ||
	    !((pg_start >= TEXT_BASE && pg_start < TEXT_BASE + pg_len * 4) ||
	      ((uint32_t)_text >= pg_start && (uint32_t)_text <= pg_end)))
		goto copy_link;

copy_piggy_link:
	/*
	 * copy piggydata binary to its link address
	 */
	memcpy(&input_data, (void *)pg_start, pg_len);
	pg_start = (uint32_t)&input_data;

copy_link:
	/* relocate to link address if necessary */
	if (offset)
		memcpy((void *)_text, (void *)(_text - offset),
				__bss_start - _text);

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	/* flush I-cache before jumping to the copied binary */
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));

	r = (unsigned int)&barebox_uncompress;
	/* call barebox_uncompress with its absolute address */
	__asm__ __volatile__(
		"mov r0, %1\n"
		"mov r1, %2\n"
		"mov pc, %0\n"
		:
		: "r"(r), "r"(pg_start), "r"(pg_len)
		: "r0", "r1");
}

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
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

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

static void barebox_uncompress(void *compressed_start, unsigned int len)
{
	void (*barebox)(void);

	if (IS_ENABLED(CONFIG_THUMB2_BAREBOX))
		barebox = (void *)(TEXT_BASE + 1);
	else
		barebox = (void *)TEXT_BASE;

	decompress((void *)compressed_start,
			len,
			NULL, NULL,
			(void *)TEXT_BASE, NULL, NULL);

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

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

void barebox_pbl(uint32_t offset)
{
}

/*
 * Board code can jump here by either returning from board_init_lowlevel
 * or by calling this function directly.
 */
void __naked __section(.text_ll_return) board_init_lowlevel_return(void)
{
	uint32_t r, addr, offset;

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

	/* relocate to link address if necessary */
	if (offset)
		memcpy((void *)_text, (void *)(_text - offset),
				__bss_start - _text);

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	/* flush I-cache before jumping to the copied binary */
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));

	r = (unsigned int)&barebox_pbl;
	/* call barebox_uncompress with its absolute address */
	__asm__ __volatile__(
		"mov r0, %1\n"
		"mov pc, %0\n"
		:
		: "r"(r), "r"(offset),
		: "r0");
}

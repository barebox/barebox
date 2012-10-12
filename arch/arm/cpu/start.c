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
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/cache.h>

#ifdef CONFIG_PBL_IMAGE
/*
 * First function in the uncompressed image. We get here from
 * the pbl.
 */
void __naked __section(.text_entry) start(void)
{
	u32 r;

	/* Setup the stack */
	r = STACK_BASE + STACK_SIZE - 16;
	__asm__ __volatile__("mov sp, %0" : : "r"(r));
	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	start_barebox();
}
#else

/*
 * First function in the image without pbl support
 */
void __naked __section(.text_entry) start(void)
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

/*
 * Board code can jump here by either returning from board_init_lowlevel
 * or by calling this function directly.
 */
void __naked board_init_lowlevel_return(void)
{
	uint32_t r, offset;

	arm_setup_stack(STACK_BASE + STACK_SIZE - 16);

	/* Get offset between linked address and runtime address */
	offset = get_runtime_offset();

	/* relocate to link address if necessary */
	if (offset)
		memcpy((void *)_text, (void *)(_text - offset),
				__bss_start - _text);

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	flush_icache();

	/* call start_barebox with its absolute address */
	r = (unsigned int)&start_barebox;
	__asm__ __volatile__("mov pc, %0" : : "r"(r));
}
#endif

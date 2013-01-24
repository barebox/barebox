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
#include <memory.h>

/*
 * First function in the uncompressed image. We get here from
 * the pbl.
 */
void __naked __section(.text_entry) start(void)
{
#ifdef CONFIG_PBL_IMAGE
	board_init_lowlevel_return();
#else
	barebox_arm_head();
#endif
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
void __naked __noreturn board_init_lowlevel_return(void)
{
	arm_setup_stack(STACK_BASE + STACK_SIZE - 16);

	setup_c();

	start_barebox();
}

/*
 * Main ARM entry point in the uncompressed image. Call this with the memory
 * region you can spare for barebox. This doesn't necessarily have to be the
 * full SDRAM. The currently running binary can be inside or outside of this
 * region. TEXT_BASE can be inside or outside of this region. boarddata will
 * be preserved and can be accessed later with barebox_arm_boarddata().
 */
void __naked __noreturn barebox_arm_entry(uint32_t membase, uint32_t memsize,
                uint32_t boarddata)
{
	board_init_lowlevel_return();
}

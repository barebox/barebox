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

unsigned long arm_stack_top;

static noinline __noreturn void __start(uint32_t membase, uint32_t memsize,
		uint32_t boarddata)
{
	unsigned long endmem = membase + memsize;

	setup_c();

	arm_stack_top = endmem;
	endmem -= STACK_SIZE; /* Stack */

	start_barebox();
}

#ifndef CONFIG_PBL_IMAGE

void __naked __section(.text_entry) start(void)
{
	barebox_arm_head();
}

/*
 * Main ARM entry point in the uncompressed image. Call this with the memory
 * region you can spare for barebox. This doesn't necessarily have to be the
 * full SDRAM. The currently running binary can be inside or outside of this
 * region. TEXT_BASE can be inside or outside of this region. boarddata will
 * be preserved and can be accessed later with barebox_arm_boarddata().
 *
 * -> membase + memsize
 *   STACK_SIZE              - stack
 *   16KiB, aligned to 16KiB - First level page table if early MMU support
 *                             is enabled
 * -> maximum end of barebox binary
 *
 * Usually a TEXT_BASE of 1MiB below your lowest possible end of memory should
 * be fine.
 */
void __naked __noreturn barebox_arm_entry(uint32_t membase, uint32_t memsize,
                uint32_t boarddata)
{
	arm_setup_stack(membase + memsize - 16);

	__start(membase, memsize, boarddata);
}
#else
/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
void __naked __section(.text_entry) start(uint32_t membase, uint32_t memsize,
                uint32_t boarddata)
{
	__start(membase, memsize, boarddata);
}
#endif

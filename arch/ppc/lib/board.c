/*
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <debug_ll.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <memory.h>
#include <init.h>
#include <net.h>
#include <asm-generic/memory_layout.h>

/************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r (ulong end_of_ram)
{
	unsigned long malloc_end;

	asm ("sync ; isync");

#ifdef CONFIG_MPC85xx
	_text_base = end_of_ram;
#endif

	/*
	 * FIXME: 128k stack size. Is this enough? should
	 *        it be configurable?
	 */
	malloc_end = (_text_base - (128 << 10)) & ~(4095);

	debug("malloc_end: 0x%08lx\n", malloc_end);
	debug("TEXT_BASE after relocation: 0x%08lx\n", _text_base);

	mem_malloc_init((void *)(malloc_end - MALLOC_SIZE), (void *)(malloc_end - 1));

	/*
	 * Setup trap handlers
	 */
	trap_init (0);

	/* Initialization complete - start the monitor */

	start_barebox();
}


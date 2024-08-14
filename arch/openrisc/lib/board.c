/*
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
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

#include <common.h>
#include <malloc.h>
#include <init.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <debug_ll.h>

/* Called from assembly */
void openrisc_start_barebox(void);

void __noreturn openrisc_start_barebox(void)
{
	mem_malloc_init((void *)(OPENRISC_SOPC_TEXT_BASE - MALLOC_SIZE),
			(void *)(OPENRISC_SOPC_TEXT_BASE - 1));

	putc_ll('>');

	start_barebox();
}

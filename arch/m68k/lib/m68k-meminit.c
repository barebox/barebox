/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Init for memory allocator on m68k/Coldfire
 */
#include <common.h>
#include <init.h>
#include <mem_malloc.h>
#include <asm/barebox-m68k.h>
#include <reloc.h>
#include <asm-generic/memory_layout.h>

/** Initialize mem allocator on M68k/Coldfire
 */
int m68k_mem_malloc_init(void)
{
	/* Pass start and end address of managed memory */

	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE));

	return 0;
}

core_initcall(m68k_mem_malloc_init);

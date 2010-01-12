/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
 *
 * This code was inspired by the GRUB2 project.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/**
 * @file
 * @brief Memory management
 */

#include <init.h>
#include <stdio.h>
#include <mem_malloc.h>
#include <asm/syslib.h>
#include <asm-generic/memory_layout.h>

/**
 * Handling of free memory
 *
 * Topics:
 * - areas used by BIOS code
 * - The 0xa0000... 0xfffff hole
 * - memory above 0x100000
 */

static int x86_mem_malloc_init(void)
{
#ifdef CONFIG_MEMORY_LAYOUT_DEFAULT
	unsigned long memory_size;

	memory_size = bios_get_memsize();
	memory_size <<= 10;	/* BIOS reports in kiB */

	/*
	 * We do not want to conflict with the kernel. So, we keep the
	 * area from 0x100000 ... 0xFFFFFF free from usage
	 */
	if (memory_size >= (15 * 1024 * 1024 + MALLOC_SIZE))
		mem_malloc_init((void*)(16 * 1024 * 1024),
				(void*)(16 * 1024 * 1024) + MALLOC_SIZE);
	else
		return -1;
#else
	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE));
#endif
	return 0;
}

core_initcall(x86_mem_malloc_init);

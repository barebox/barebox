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
 *
 */

/**
 * @file
 * @brief Memory management
 */

#include <common.h>
#include <init.h>
#include <stdio.h>
#include <memory.h>
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

int x86_start_barebox(void)
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
				(void*)(16 * 1024 * 1024 + MALLOC_SIZE - 1));
	else
		return -1;
#else
	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE - 1));
#endif
	start_barebox();
}

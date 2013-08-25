/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
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
#include <pbl.h>
#include <init.h>
#include <sizes.h>
#include <string.h>
#include <asm/sections.h>
#include <asm-generic/memory_layout.h>
#include <debug_ll.h>

extern void *input_data;
extern void *input_data_end;

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

void pbl_main_entry(void);

static unsigned long *ttb;

static void barebox_uncompress(void *compressed_start, unsigned int len)
{
	/* set 128 KiB at the end of the MALLOC_BASE for early malloc */
	free_mem_ptr = MALLOC_BASE + MALLOC_SIZE - SZ_128K;
	free_mem_end_ptr = free_mem_ptr + SZ_128K;

	ttb = (void *)((free_mem_ptr - 0x4000) & ~0x3fff);

	pbl_barebox_uncompress((void*)TEXT_BASE, compressed_start, len);
}

void __section(.text_entry) pbl_main_entry(void)
{
	u32 pg_start, pg_end, pg_len;
	void (*barebox)(void);

	puts_ll("pbl_main_entry()\n");

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	pg_start = (u32)&input_data;
	pg_end = (u32)&input_data_end;
	pg_len = pg_end - pg_start;

	barebox_uncompress(&input_data, pg_len);

	barebox = (void *)TEXT_BASE;
	barebox();
}

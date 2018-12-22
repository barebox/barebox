// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <common.h>
#include <pbl.h>
#include <init.h>
#include <linux/sizes.h>
#include <string.h>
#include <asm/sections.h>
#include <asm-generic/memory_layout.h>
#include <debug_ll.h>

extern void *input_data;
extern void *input_data_end;

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

void pbl_main_entry(void *fdt, void *fdt_end);

static unsigned long *ttb;

static void barebox_uncompress(void *compressed_start, unsigned int len)
{
	/* set 128 KiB at the end of the MALLOC_BASE for early malloc */
	free_mem_ptr = MALLOC_BASE + MALLOC_SIZE - SZ_128K;
	free_mem_end_ptr = free_mem_ptr + SZ_128K;

	ttb = (void *)((free_mem_ptr - 0x4000) & ~0x3fff);

	pbl_barebox_uncompress((void*)TEXT_BASE, compressed_start, len);
}

void __section(.text_entry) pbl_main_entry(void *fdt, void *fdt_end)
{
	u32 pg_start, pg_end, pg_len, fdt_len;
	void *fdt_new;
	void (*barebox)(void *fdt, u32 fdt_len);

	puts_ll("pbl_main_entry()\n");

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	pg_start = (u32)&input_data;
	pg_end = (u32)&input_data_end;
	pg_len = pg_end - pg_start;

	barebox_uncompress(&input_data, pg_len);

	fdt_len = (u32)fdt_end - (u32)fdt;
	fdt_new = (void *)PAGE_ALIGN_DOWN(STACK_BASE - fdt_len);
	memcpy(fdt_new, fdt, fdt_len);

	barebox = (void *)TEXT_BASE;
	barebox(fdt_new, fdt_len);
}

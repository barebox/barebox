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
#include <asm/unaligned.h>

extern void *input_data;
extern void *input_data_end;

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

void barebox_pbl_start(void *fdt, void *fdt_end, u32 ram_size);

void __section(.text_entry) barebox_pbl_start(void *fdt, void *fdt_end,
					   u32 ram_size)
{
	u32 piggy_len, fdt_len;
	void *fdt_new;
	void (*barebox)(void *fdt, u32 ram_size);

	puts_ll("barebox_pbl_start()\n");

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	/* set 128 KiB at the end of the MALLOC_BASE for early malloc */
	free_mem_ptr = TEXT_BASE - SZ_128K;
	free_mem_end_ptr = free_mem_ptr + SZ_128K;

	piggy_len = (unsigned long)&input_data_end - (unsigned long)&input_data;

	pbl_barebox_uncompress((void *)TEXT_BASE, &input_data, piggy_len);

	fdt_len = (unsigned long)fdt_end - (unsigned long)fdt;
	if (!fdt_len) {
		fdt_len = get_unaligned_be32((void *)((unsigned long)fdt + 4));
	}
	fdt_new = (void *)PAGE_ALIGN_DOWN(TEXT_BASE - MALLOC_SIZE - STACK_SIZE - fdt_len);
	memcpy(fdt_new, fdt, fdt_len);

	barebox = (void *)TEXT_BASE;
	barebox(fdt_new, ram_size);
}

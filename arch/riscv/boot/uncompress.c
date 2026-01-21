// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010-2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>

/* uncompress.c - uncompressor code for self extracing pbl image */

#define pr_fmt(fmt) "uncompress.c: " fmt

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <pbl.h>
#include <pbl/mmu.h>
#include <asm/barebox-riscv.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/unaligned.h>
#include <asm/mmu.h>
#include <asm/irq.h>
#include <elf.h>

#include <debug_ll.h>

#include "entry.h"

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

void __noreturn barebox_pbl_start(unsigned long membase, unsigned long memsize,
				  void *fdt)
{
	uint32_t pg_len, uncompressed_len;
	void __noreturn (*barebox)(unsigned long, unsigned long, void *);
	unsigned long endmem = membase + memsize;
	unsigned long barebox_base;
	void *pg_start, *pg_end;
	unsigned long pc = get_pc();
	struct elf_image elf;
	int ret;

	irq_init_vector(riscv_mode());

	/* piggy data is not relocated, so determine the bounds now */
	pg_start = runtime_address(input_data);
	pg_end = runtime_address(input_data_end);
	pg_len = pg_end - pg_start;
	uncompressed_len = input_data_len();

	/*
	 * If we run from inside the memory just relocate the binary
	 * to the current address. Otherwise it may be a readonly location.
	 * Copy and relocate to the start of the memory in this case.
	 */
	if (pc > membase && pc - membase < memsize)
		relocate_to_current_adr();
	else
		relocate_to_adr(membase);

	barebox_base = riscv_mem_barebox_image(membase, endmem,
					       uncompressed_len + MAX_BSS_SIZE);

	setup_c();

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	free_mem_ptr = riscv_mem_early_malloc(membase, endmem);
	free_mem_end_ptr = riscv_mem_early_malloc_end(membase, endmem);

	/*
	 * Enable MMU early to enable caching for faster decompression.
	 * This creates an initial identity mapping that will be refined
	 * later based on ELF segments.
	 */
	if (IS_ENABLED(CONFIG_MMU))
		mmu_early_enable(membase, memsize, barebox_base);

	pr_debug("uncompressing barebox binary at 0x%p (size 0x%08x) to 0x%08lx (uncompressed size: 0x%08x)\n",
			pg_start, pg_len, barebox_base, uncompressed_len);

	pbl_barebox_uncompress((void*)barebox_base, pg_start, pg_len);

	sync_caches_for_execution();

	ret = elf_open_binary_into(&elf, (void *)barebox_base);
	if (ret)
		panic("Failed to open ELF binary: %d\n", ret);

	ret = elf_load_inplace(&elf);
	if (ret)
		panic("Failed to relocate ELF: %d\n", ret);

	/*
	 * Now that the ELF image is relocated, we know the exact addresses
	 * of all segments. Set up MMU with proper permissions based on
	 * ELF segment flags (PF_R/W/X).
	 */
	ret = pbl_mmu_setup_from_elf(&elf, membase, memsize);
	if (ret)
		panic("Failed to setup memory protection from ELF: %d\n", ret);

	barebox = (void *)(unsigned long)elf.entry;

	pr_debug("jumping to uncompressed image at 0x%p. dtb=0x%p\n", barebox, fdt);

	barebox(membase, memsize, fdt);
}

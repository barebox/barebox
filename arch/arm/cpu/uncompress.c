// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010-2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>

/* uncompress.c - uncompressor code for self extracing pbl image */

#define pr_fmt(fmt) "uncompress.c: " fmt

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <pbl.h>
#include <pbl/handoff-data.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/secure.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <asm/unaligned.h>
#include <compressed-dtb.h>

#include <debug_ll.h>

#include "entry.h"

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

extern unsigned char input_data[];
extern unsigned char input_data_end[];

static void add_handoff_data(void *boarddata)
{
	if (!boarddata)
		return;
	if (blob_is_fdt(boarddata)) {
		handoff_data_add(HANDOFF_DATA_INTERNAL_DT, boarddata,
				 get_unaligned_be32(boarddata + 4));
	} else if (blob_is_compressed_fdt(boarddata)) {
		struct barebox_boarddata_compressed_dtb *bd = boarddata;

		handoff_data_add(HANDOFF_DATA_INTERNAL_DT_Z, boarddata,
				 bd->datalen + sizeof(*bd));
	} else if (blob_is_arm_boarddata(boarddata)) {
		handoff_data_add(HANDOFF_DATA_BOARDDATA, boarddata,
				 sizeof(struct barebox_arm_boarddata));
	}
}

void __noreturn barebox_pbl_start(unsigned long membase, unsigned long memsize,
				  void *boarddata)
{
	uint32_t pg_len, uncompressed_len;
	void __noreturn (*barebox)(unsigned long, unsigned long, void *);
	unsigned long endmem = membase + memsize;
	unsigned long barebox_base;
	void *pg_start, *pg_end;
	unsigned long pc = get_pc();
	void *handoff_data;

	/* piggy data is not relocated, so determine the bounds now */
	pg_start = runtime_address(input_data);
	pg_end = runtime_address(input_data_end);

	/*
	 * If we run from inside the memory just relocate the binary
	 * to the current address. Otherwise it may be a readonly location.
	 * Copy and relocate to the start of the memory in this case.
	 */
	if (pc > membase && pc - membase < memsize)
		relocate_to_current_adr();
	else
		relocate_to_adr(membase);

	pg_len = pg_end - pg_start;
	uncompressed_len = get_unaligned((const u32 *)(pg_start + pg_len - 4));

	setup_c();

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	if (IS_ENABLED(CONFIG_MMU))
		mmu_early_enable(membase, memsize);

	/* Add handoff data now, so arm_mem_barebox_image takes it into account */
	add_handoff_data(boarddata);

	barebox_base = arm_mem_barebox_image(membase, endmem,
					     uncompressed_len, NULL);

	handoff_data = (void *)barebox_base + uncompressed_len + MAX_BSS_SIZE;

	free_mem_ptr = barebox_base - ARM_MEM_EARLY_MALLOC_SIZE;
	free_mem_end_ptr = barebox_base;

	pr_debug("uncompressing barebox binary at 0x%p (size 0x%08x) to 0x%08lx (uncompressed size: 0x%08x)\n",
			pg_start, pg_len, barebox_base, uncompressed_len);

	pbl_barebox_uncompress((void*)barebox_base, pg_start, pg_len);

	handoff_data_move(handoff_data);

	sync_caches_for_execution();

	if (IS_ENABLED(CONFIG_THUMB2_BAREBOX))
		barebox = (void *)(barebox_base + 1);
	else
		barebox = (void *)barebox_base;

	pr_debug("jumping to uncompressed image at 0x%p\n", barebox);

	if (IS_ENABLED(CONFIG_CPU_V7) && boot_cpu_mode() == HYP_MODE)
		armv7_switch_to_hyp();

	barebox(membase, memsize, handoff_data);
}

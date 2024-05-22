// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#define pr_fmt(fmt) "start.c: " fmt

#ifdef CONFIG_DEBUG_INITCALLS
#define DEBUG
#endif

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <of.h>
#include <asm/barebox-riscv.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/unaligned.h>
#include <linux/kasan.h>
#include <memory.h>
#include <uncompress.h>
#include <malloc.h>
#include <compressed-dtb.h>
#include <asm/irq.h>

#include <debug_ll.h>

#include "entry.h"

unsigned long riscv_stack_top;
static unsigned long riscv_barebox_size;
static unsigned long riscv_endmem;
static void *barebox_boarddata;
static unsigned long barebox_boarddata_size;

void *barebox_riscv_boot_dtb(void)
{
	void *dtb;
	int ret;
	struct barebox_boarddata_compressed_dtb *compressed_dtb;
	static void *boot_dtb;

	if (boot_dtb)
		return boot_dtb;

	if (barebox_boarddata && blob_is_fdt(barebox_boarddata)) {
		pr_debug("%s: using barebox_boarddata\n", __func__);
		return barebox_boarddata;
	}

	if (!fdt_blob_can_be_decompressed(barebox_boarddata))
		return NULL;

	compressed_dtb = barebox_boarddata;

	pr_debug("%s: using compressed_dtb\n", __func__);

	dtb = malloc(ALIGN(compressed_dtb->datalen_uncompressed, 4));
	if (!dtb)
		return NULL;

	ret = uncompress(compressed_dtb->data, compressed_dtb->datalen,
			 NULL, NULL, dtb, NULL, NULL);
	if (ret) {
		pr_err("uncompressing dtb failed\n");
		free(dtb);
		return NULL;
	}

	boot_dtb = dtb;

	return boot_dtb;
}

static inline unsigned long riscv_mem_boarddata(unsigned long membase,
						unsigned long endmem,
						unsigned long size)
{
	unsigned long mem;

	mem = riscv_mem_barebox_image(membase, endmem, riscv_barebox_size);
	mem -= ALIGN(size, 64);

	return mem;
}

unsigned long riscv_mem_ramoops_get(void)
{
	return riscv_mem_ramoops(0, riscv_stack_top);
}
EXPORT_SYMBOL_GPL(riscv_mem_ramoops_get);

unsigned long riscv_mem_endmem_get(void)
{
	return riscv_endmem;
}
EXPORT_SYMBOL_GPL(riscv_mem_endmem_get);

static int barebox_memory_areas_init(void)
{
	if(barebox_boarddata)
		request_barebox_region("board data", (unsigned long)barebox_boarddata,
				       barebox_boarddata_size);

	return 0;
}
device_initcall(barebox_memory_areas_init);

/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
__noreturn __no_sanitize_address __section(.text_entry)
void barebox_non_pbl_start(unsigned long membase, unsigned long memsize,
			   void *boarddata)
{
	unsigned long endmem = membase + memsize;
	unsigned long malloc_start, malloc_end;
	unsigned long barebox_size = barebox_image_size + MAX_BSS_SIZE;
	unsigned long barebox_base = riscv_mem_barebox_image(membase, endmem, barebox_size);

	relocate_to_current_adr();

	setup_c();

	barrier();

	irq_init_vector(riscv_mode());

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	riscv_endmem = endmem;
	riscv_stack_top = riscv_mem_stack_top(membase, endmem);
	riscv_barebox_size = barebox_size;
	malloc_end = barebox_base;

	if (boarddata) {
		uint32_t totalsize = 0;
		const char *name;

		if (blob_is_fdt(boarddata)) {
			totalsize = get_unaligned_be32(boarddata + 4);
			name = "DTB";
		} else if (blob_is_compressed_fdt(boarddata)) {
			struct barebox_boarddata_compressed_dtb *bd = boarddata;
			totalsize = bd->datalen + sizeof(*bd);
			name = "Compressed DTB";
		}

		if (totalsize) {
			unsigned long mem = riscv_mem_boarddata(membase, endmem, totalsize);
			pr_debug("found %s in boarddata, copying to 0x%08lx\n", name, mem);
			barebox_boarddata = memcpy((void *)mem, boarddata, totalsize);
			barebox_boarddata_size = totalsize;
			malloc_end = mem;
		}
	}

	/*
	 * Maximum malloc space is the Kconfig value if given
	 * or 1GB.
	 */
	if (MALLOC_SIZE > 0) {
		malloc_start = malloc_end - MALLOC_SIZE;
		if (malloc_start < membase)
			malloc_start = membase;
	} else {
		malloc_start = malloc_end - (malloc_end - membase) / 2;
		if (malloc_end - malloc_start > SZ_1G)
			malloc_start = malloc_end - SZ_1G;
	}

	pr_debug("initializing malloc pool at 0x%08lx (size 0x%08lx)\n",
			malloc_start, malloc_end - malloc_start);

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);

	pr_debug("starting barebox...\n");

	start_barebox();
}

void start(unsigned long membase, unsigned long memsize, void *boarddata);
/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
void __no_sanitize_address __section(.text_entry) start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	barebox_non_pbl_start(membase, memsize, boarddata);
}

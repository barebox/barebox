// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "barebox-proper.c: " fmt

#ifdef CONFIG_DEBUG_INITCALLS
#define DEBUG
#endif

#include <pbl/handoff-data.h>
#include <pbl/barebox-proper.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <barebox.h>
#include <compressed-dtb.h>
#include <init.h>
#include <linux/kasan.h>
#include <linux/printk.h>
#include <linux/sizes.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <uncompress.h>

void *barebox_boot_dtb(void)
{
	void *dtb;
	int ret = 0;
	struct barebox_boarddata_compressed_dtb *compressed_dtb;
	static void *boot_dtb;
	void *blob;
	size_t size;

	if (boot_dtb)
		return boot_dtb;

	blob = handoff_data_get_entry(HANDOFF_DATA_INTERNAL_DT, &size);
	if (blob)
		return blob;

	blob = handoff_data_get_entry(HANDOFF_DATA_INTERNAL_DT_Z, &size);
	if (!blob)
		return NULL;

	if (!fdt_blob_can_be_decompressed(blob))
		return NULL;

	compressed_dtb = blob;

	pr_debug("%s: using compressed_dtb\n", __func__);

	dtb = malloc(ALIGN(compressed_dtb->datalen_uncompressed, 4));
	if (!dtb)
		return NULL;

	if (IS_ENABLED(CONFIG_IMAGE_COMPRESSION_NONE))
		memcpy(dtb, compressed_dtb->data,
		       compressed_dtb->datalen_uncompressed);
	else
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

static int barebox_memory_areas_init(void)
{
	if (kasan_enabled())
		request_sdram_region("kasan shadow", kasan_shadow_base,
				     mem_malloc_start() - kasan_shadow_base,
				     MEMTYPE_BOOT_SERVICES_DATA, MEMATTRS_RW);

	return 0;
}
device_initcall(barebox_memory_areas_init);

void __weak arch_adjust_malloc_end(unsigned long *malloc_end)
{
}

static unsigned long barebox_malloc_start(unsigned long membase,
					  unsigned long memsize,
					  unsigned long malloc_end)
{
#ifdef CONFIG_BAREBOX_MEMORY_OFFSET
	return barebox_malloc_base(membase, memsize);
#else
	unsigned long malloc_start;

	/* TODO: migrate ARM32 to barebox_malloc_base(), once legacy xload
	 * code has been migrated to loadables; On boards like the beaglebone,
	 * a separate first stage barebox loads the second stage into RAM
	 * and executes it from there, so reusing the malloc area in second
	 * stage PBL will mangle the second stage code.
	 */
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

	return malloc_start;
#endif
}

/* entered from the PBL, which has set up the stack already */
void __noreturn barebox_non_pbl_start(unsigned long membase,
				      unsigned long memsize,
				      struct handoff_data *hd)
{
	unsigned long malloc_start, malloc_end;

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	malloc_end = (unsigned long)__image_start;
	arch_adjust_malloc_end(&malloc_end);
	malloc_start = barebox_malloc_start(membase, memsize, malloc_end);

	pr_debug("initializing malloc pool at 0x%08lx (size 0x%08lx)\n",
			malloc_start, malloc_end - malloc_start);

	kasan_init(membase, memsize, malloc_start - (memsize >> KASAN_SHADOW_SCALE_SHIFT));

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);

	handoff_data_set(hd);

	pr_debug("starting barebox...\n");

	start_barebox();
}

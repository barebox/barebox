// SPDX-License-Identifier: GPL-2.0
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2019 Kalray Inc.
 */

#include <elf.h>
#include <boot.h>
#include <init.h>
#include <bootm.h>
#include <binfmt.h>
#include <common.h>
#include <libfile.h>
#include <linux/kernel.h>

#include <asm/cache.h>

typedef void __noreturn (*boot_func_entry)(unsigned long, void *);

static int do_boot_entry(struct image_data *data, boot_func_entry entry,
			 void *fdt_load_addr)
{
	int ret;

	printf("starting elf (entry at %p)\n", entry);

	if (data->dryrun)
		return 0;

	ret = of_overlay_load_firmware();
	if (ret)
		return ret;

	shutdown_barebox();

	/* Synchronize I-cache with D-cache */
	sync_caches_for_execution();

	/**
	 * Parameters passing
	 * r0: boot magic
	 * r1: device tree pointer
	 */
	entry(LINUX_BOOT_PARAM_MAGIC, (void *) fdt_load_addr);

	/* should never return ! */
	panic("Returned from boot program !\n");

	return -EINVAL;
}

static int do_boot_elf(struct image_data *data, struct elf_image *elf)
{
	const struct resource *initrd_res;
	int ret;
	void *fdt;
	boot_func_entry entry;
	unsigned long load_addr, initrd_address;

	ret = elf_load(elf);
	if (ret)
		return ret;

	/* load initrd after the elf */
	load_addr = PAGE_ALIGN((unsigned long) elf->high_addr);
	if (data->initrd_address != UIMAGE_INVALID_ADDRESS)
		initrd_address = data->initrd_address;
	else
		initrd_address = load_addr;

	initrd_res = bootm_load_initrd(data, initrd_address);
	if (IS_ERR(initrd_res)) {
		printf("Failed to load initrd\n");
		return PTR_ERR(initrd_res);
	} else if (initrd_res) {
		printf("Loading initrd at 0x%lx\n", initrd_address);
		if (data->initrd_address == UIMAGE_INVALID_ADDRESS) {
			load_addr += resource_size(initrd_res);
			load_addr = PAGE_ALIGN(load_addr);
		}
	}

	fdt = bootm_get_devicetree(data);
	if (!fdt)
		fdt = ERR_PTR(-EINVAL);
	if (IS_ERR(fdt)) {
		printf("Failed to load dtb\n");
		return PTR_ERR(fdt);
	}

	printf("Loading device tree at %lx\n", load_addr);
	/* load device tree after the initrd if any */
	ret = bootm_load_devicetree(data, fdt, load_addr);
	if (ret) {
		printf("Failed to load device tree: %d\n", ret);
		goto err_free_fdt;
	}

	entry = (boot_func_entry) elf->entry;

	ret = do_boot_entry(data, entry, fdt);

err_free_fdt:
	free(fdt);

	return ret;
}

static int do_bootm_elf(struct image_data *data)
{
	struct elf_image *elf;
	int ret;

	if (data->fit_kernel)
		elf = elf_open_binary((void *) data->fit_kernel);
	else
		elf = elf_open(data->os_file);

	if (IS_ERR(elf))
		return PTR_ERR(elf);

	ret = do_boot_elf(data, elf);

	elf_close(elf);

	return ret;
}

static struct image_handler elf_handler = {
	.name = "ELF",
	.bootm = do_bootm_elf,
	.filetype = filetype_elf,
};

static struct binfmt_hook binfmt_elf_hook = {
	.type = filetype_elf,
	.exec = "bootm",
};

static int kvx_register_image_handler(void)
{
	register_image_handler(&elf_handler);

	binfmt_register(&binfmt_elf_hook);

	return 0;
}

late_initcall(kvx_register_image_handler);

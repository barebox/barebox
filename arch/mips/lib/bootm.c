// SPDX-License-Identifier: GPL-2.0-only

#include <boot.h>
#include <bootm.h>
#include <common.h>
#include <libfile.h>
#include <malloc.h>
#include <init.h>
#include <fs.h>
#include <errno.h>
#include <binfmt.h>
#include <restart.h>

#include <asm/byteorder.h>
#include <asm/io.h>

static int do_bootm_barebox(struct image_data *data)
{
	void (*barebox)(int, void *);
	void *fdt = NULL;

	barebox = read_file(data->os_file, NULL);
	if (!barebox)
		return -EINVAL;

	if (data->oftree_file) {
		fdt = bootm_get_devicetree(data);
		if (IS_ERR(fdt)) {
			pr_err("Failed to load dtb\n");
			return PTR_ERR(fdt);
		}
	}

	if (data->dryrun) {
		free(barebox);
		return 0;
	}

	shutdown_barebox();

	barebox(-2, fdt);

	restart_machine(0);
}

static struct image_handler barebox_handler = {
	.name = "MIPS barebox",
	.bootm = do_bootm_barebox,
	.filetype = filetype_mips_barebox,
};

static struct binfmt_hook binfmt_barebox_hook = {
	.type = filetype_mips_barebox,
	.exec = "bootm",
};

static int do_bootm_elf(struct image_data *data)
{
	void (*entry)(int, void *);
	void *fdt;
	struct elf_image *elf;
	int ret = 0;

	elf = elf_open(data->os_file);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	ret = elf_load(elf);
	if (ret)
		goto err;

	fdt = bootm_get_devicetree(data);
	if (IS_ERR(fdt)) {
		pr_err("Failed to load dtb\n");
		ret = PTR_ERR(fdt);
		goto bootm_free_fdt;
	}

	pr_info("Starting application at 0x%08lx, dts 0x%p...\n",
		elf->entry, data->of_root_node);

	if (data->dryrun)
		goto bootm_free_fdt;

	ret = of_overlay_load_firmware();
	if (ret)
		goto bootm_free_fdt;

	shutdown_barebox();

	entry = (void *) (unsigned long) elf->entry;

	entry(-2, fdt);

	pr_err("ELF application terminated\n");
	ret = -EINVAL;

bootm_free_fdt:
	free(fdt);
err:
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

static int mips_register_image_handler(void)
{
	register_image_handler(&barebox_handler);
	binfmt_register(&binfmt_barebox_hook);

	register_image_handler(&elf_handler);
	binfmt_register(&binfmt_elf_hook);

	return 0;
}
late_initcall(mips_register_image_handler);

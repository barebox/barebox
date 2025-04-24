// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "ELF: " fmt

#include <bootm.h>
#include <elf.h>
#include <common.h>
#include <init.h>
#include <errno.h>

static int do_bootm_elf(struct image_data *data)
{
	void (*fn)(unsigned long x0, unsigned long x1, unsigned long x2,
		   unsigned long x3);
	struct elf_image *elf;
	int ret;

	elf = elf_open(data->os_file);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	if (elf_hdr_e_machine(elf, elf->hdr_buf) != ELF_ARCH) {
		pr_err("Unsupported machine: 0x%02x, but 0x%02x expected\n",
		       elf_hdr_e_machine(elf, elf->hdr_buf), ELF_ARCH);

		ret = -EINVAL;
		goto err;
	}

	ret = elf_load(elf);
	if (ret)
		goto err;

	if (data->dryrun) {
		ret = 0;
		goto err;
	}

	ret = of_overlay_load_firmware();
	if (ret)
		goto err;

	shutdown_barebox();

	fn = (void *) (unsigned long) data->os_address;

	fn(0, 0, 0, 0);

	pr_err("ELF application terminated\n");
	ret = -EINVAL;

err:
	elf_close(elf);

	return ret;
}

static struct image_handler elf_handler = {
	.name = "ELF",
	.bootm = do_bootm_elf,
	.filetype = filetype_elf,
};

static int arm_register_elf_image_handler(void)
{
	return register_image_handler(&elf_handler);
}
late_initcall(arm_register_elf_image_handler);

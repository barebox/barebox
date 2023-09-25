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
	struct elf_image *elf = data->elf;
	int ret;

	if (elf_hdr_e_machine(elf, elf->hdr_buf) != ELF_ARCH) {
		pr_err("Unsupported machine: 0x%02x, but 0x%02x expected\n",
		       elf_hdr_e_machine(elf, elf->hdr_buf), ELF_ARCH);

		return -EINVAL;
	}

	ret = bootm_load_os(data, data->os_address);
	if (ret)
		return ret;

	if (data->dryrun)
		return ret;

	ret = of_overlay_load_firmware();
	if (ret)
		return ret;

	shutdown_barebox();

	fn = (void *) (unsigned long) data->os_address;

	fn(0, 0, 0, 0);

	pr_err("ELF application terminated\n");
	return -EINVAL;
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

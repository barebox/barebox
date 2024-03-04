// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2018 Sascha Hauer <s.hauer@pengutronix.de>

#include <common.h>
#include <bootm.h>
#include <asm/system.h>

static int do_bootm_linux(struct image_data *data)
{
	void (*fn)(unsigned long a0, unsigned long dtb, unsigned long a2);
	phys_addr_t devicetree;
	long hartid = riscv_hartid();
	int ret;

	fn = booti_load_image(data, &devicetree);
	if (IS_ERR(fn))
		return PTR_ERR(fn);

	if (data->dryrun)
		return 0;

	ret = of_overlay_load_firmware();
	if (ret)
		return ret;

	shutdown_barebox();

	fn(hartid >= 0 ? hartid : 0, devicetree, 0);

	return -EINVAL;
}

static struct image_handler riscv_linux_handler = {
        .name = "RISC-V Linux image",
        .bootm = do_bootm_linux,
        .filetype = filetype_riscv_linux_image,
};

static struct image_handler riscv_linux_efi_handler = {
        .name = "RISC-V Linux/EFI image",
        .bootm = do_bootm_linux,
        .filetype = filetype_riscv_efi_linux_image,
};

static struct image_handler riscv_fit_handler = {
	.name = "FIT image",
	.bootm = do_bootm_linux,
	.filetype = filetype_oftree,
};

static struct image_handler riscv_barebox_handler = {
        .name = "RISC-V barebox image",
        .bootm = do_bootm_linux,
        .filetype = filetype_riscv_barebox_image,
};

static int riscv_register_image_handler(void)
{
	register_image_handler(&riscv_linux_handler);
	register_image_handler(&riscv_linux_efi_handler);
	register_image_handler(&riscv_barebox_handler);

	if (IS_ENABLED(CONFIG_FITIMAGE))
		register_image_handler(&riscv_fit_handler);

	return 0;
}
late_initcall(riscv_register_image_handler);

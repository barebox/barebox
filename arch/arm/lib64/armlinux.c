// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2018 Sascha Hauer <s.hauer@pengutronix.de>

#include <common.h>
#include <memory.h>
#include <init.h>
#include <bootm.h>
#include <asm/boot.h>
#include <efi/efi-mode.h>

static int do_bootm_linux(struct image_data *data)
{
	void *image;
	phys_addr_t devicetree = 0;
	int ret;

	image = booti_load_image(data, &devicetree);
	if (IS_ERR(image))
		return PTR_ERR(image);

	if (data->dryrun)
		return 0;

	ret = of_overlay_load_firmware();
	if (ret)
		return ret;

	shutdown_barebox();

	jump_to_linux(image, devicetree);

	return -EINVAL;
}

static struct image_handler aarch64_linux_handler = {
        .name = "ARM aarch64 Linux image",
        .bootm = do_bootm_linux,
        .filetype = filetype_arm64_linux_image,
};

static struct image_handler aarch64_linux_efi_handler = {
        .name = "ARM aarch64 Linux/EFI image",
        .bootm = do_bootm_linux,
        .filetype = filetype_arm64_efi_linux_image,
};

static int do_bootm_barebox(struct image_data *data)
{
	void (*fn)(unsigned long x0, unsigned long x1, unsigned long x2,
		       unsigned long x3);
	resource_size_t start, end;
	unsigned long barebox;
	int ret;

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		goto out;

	barebox = PAGE_ALIGN(start);

	ret = bootm_load_os(data, barebox);
	if (ret)
		goto out;

	printf("Loaded barebox image to 0x%08lx\n", barebox);

	shutdown_barebox();

	fn = (void *)barebox;

	fn(0, 0, 0, 0);

	ret = -EINVAL;

out:
	return ret;
}

static struct image_handler aarch64_barebox_handler = {
        .name = "ARM aarch64 barebox image",
        .bootm = do_bootm_barebox,
        .filetype = filetype_arm_barebox,
};

static int aarch64_register_image_handler(void)
{
	if (efi_is_payload())
		return 0;

	register_image_handler(&aarch64_linux_efi_handler);
	register_image_handler(&aarch64_linux_handler);
	register_image_handler(&aarch64_barebox_handler);

	return 0;
}
late_initcall(aarch64_register_image_handler);

/*
 * Copyright (C) 2018 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <boot.h>
#include <common.h>
#include <environment.h>
#include <image.h>
#include <fs.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <of.h>
#include <init.h>
#include <bootm.h>
#include <linux/list.h>
#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

static int do_bootm_linux(struct image_data *data)
{
	void (*fn)(unsigned long dtb, unsigned long x1, unsigned long x2,
		       unsigned long x3);
	resource_size_t start, end;
	unsigned long text_offset, image_size, devicetree, kernel;
	int ret;
	void *fdt;

	text_offset = le64_to_cpup(data->os_header + 8);
	image_size = le64_to_cpup(data->os_header + 16);

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		goto out;

	kernel = ALIGN(start, SZ_2M) + text_offset;

	ret = bootm_load_os(data, kernel);
	if (ret)
		goto out;

	devicetree = ALIGN(kernel + image_size, PAGE_SIZE);

	fdt = bootm_get_devicetree(data);
	if (IS_ERR(fdt)) {
		ret = PTR_ERR(fdt);
		goto out;
	}

	ret = bootm_load_devicetree(data, fdt, devicetree);

	free(fdt);

	if (ret)
		goto out;

	printf("Loaded kernel to 0x%08lx, devicetree at 0x%08lx\n",
	       kernel, devicetree);

	shutdown_barebox();

	fn = (void *)kernel;

	fn(devicetree, 0, 0, 0);

	ret = -EINVAL;

out:
	return ret;
}

static struct image_handler aarch64_linux_handler = {
        .name = "ARM aarch64 Linux image",
        .bootm = do_bootm_linux,
        .filetype = filetype_arm64_linux_image,
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

	barebox = start;

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
	register_image_handler(&aarch64_linux_handler);
	register_image_handler(&aarch64_barebox_handler);

	return 0;
}
late_initcall(aarch64_register_image_handler);

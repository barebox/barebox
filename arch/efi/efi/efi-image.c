/*
 * efi-image.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <sizes.h>
#include <memory.h>
#include <command.h>
#include <magicvar.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <efi.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <boot.h>
#include <fs.h>
#include <binfmt.h>
#include <wchar.h>
#include <mach/efi.h>
#include <mach/efi-device.h>

static int efi_execute_image(const char *file)
{
	void *exe;
	size_t size;
	efi_handle_t handle;
	efi_status_t efiret;
	const char *options;
	efi_loaded_image_t *loaded_image;

	exe = read_file(file, &size);
	if (!exe)
		return -EINVAL;

	efiret = BS->load_image(false, efi_parent_image, efi_device_path, exe, size,
			&handle);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to LoadImage: %s\n", efi_strerror(efiret));
		return -efi_errno(efiret);;
	};

	efiret = BS->open_protocol(handle, &efi_loaded_image_protocol_guid,
			(void **)&loaded_image,
			efi_parent_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	options = linux_bootargs_get();
	loaded_image->load_options = strdup_char_to_wchar(options);
	loaded_image->load_options_size = (strlen(options) + 1) * sizeof(wchar_t);

	efiret = BS->start_image(handle, NULL, NULL);

	efi_connect_all();
	efi_register_devices();

	return 0;
}

static int do_bootm_efi(struct image_data *data)
{
	return efi_execute_image(data->os_file);
}

static struct image_handler efi_handle_tr = {
	.name = "EFI Application",
	.bootm = do_bootm_efi,
	.filetype = filetype_exe,
};

static int efi_execute(struct binfmt_hook *b, char *file, int argc, char **argv)
{
	return efi_execute_image(file);
}

static struct binfmt_hook binfmt_efi_hook = {
	.type = filetype_exe,
	.hook = efi_execute,
};

static int efi_register_image_handler(void)
{
	register_image_handler(&efi_handle_tr);
	binfmt_register(&binfmt_efi_hook);

	return 0;
}
late_initcall(efi_register_image_handler);

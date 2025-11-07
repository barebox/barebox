// SPDX-License-Identifier: GPL-2.0-only
/*
 * image.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#define pr_fmt(fmt) "efi-image: " fmt

#include <clock.h>
#include <common.h>
#include <linux/sizes.h>
#include <memory.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <efi.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <bootargs.h>
#include <bootm.h>
#include <fs.h>
#include <libfile.h>
#include <binfmt.h>
#include <wchar.h>
#include <efi/efi-payload.h>
#include <efi/efi-device.h>

#include "image.h"
#include "setup_header.h"

static void *efi_read_file(const char *file, size_t *size)
{
	efi_physical_addr_t mem;
	efi_status_t efiret;
	struct stat s;
	char *buf;
	ssize_t ret;

	ret = stat(file, &s);
	if (ret)
		return NULL;

	efiret = BS->allocate_pages(EFI_ALLOCATE_ANY_PAGES,
				    EFI_LOADER_CODE,
				    DIV_ROUND_UP(s.st_size, EFI_PAGE_SIZE),
				    &mem);
	if (EFI_ERROR(efiret)) {
		errno = efi_errno(efiret);
		return NULL;
	}

	buf = efi_phys_to_virt(mem);

	ret = read_file_into_buf(file, buf, s.st_size);
	if (ret < 0)
		return NULL;

	*size = ret;
	return buf;
}

int efi_load_image(const char *file, struct efi_loaded_image **loaded_image,
		   efi_handle_t *h)
{
	void *exe;
	size_t size;
	efi_handle_t handle;
	efi_status_t efiret = EFI_SUCCESS;

	exe = efi_read_file(file, &size);
	if (!exe)
		return -errno;

	efiret = BS->load_image(false, efi_parent_image, efi_device_path, exe, size,
			&handle);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to LoadImage: %s\n", efi_strerror(efiret));
		goto out;
	};

	efiret = BS->open_protocol(handle, &efi_loaded_image_protocol_guid,
			(void **)loaded_image,
			efi_parent_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to OpenProtocol: %s\n", efi_strerror(efiret));
		BS->unload_image(handle);
		goto out;
	}

	*h = handle;
out:
	BS->free_pages(efi_virt_to_phys(exe), DIV_ROUND_UP(size, EFI_PAGE_SIZE));
	return -efi_errno(efiret);
}

static bool is_linux_image(enum filetype filetype, const void *base)
{
	if (IS_ENABLED(CONFIG_X86) && is_x86_setup_header(base))
		return true;

	if (IS_ENABLED(CONFIG_ARM64) &&
	    filetype == filetype_arm64_efi_linux_image)
		return true;

	return false;
}

int efi_execute_image(efi_handle_t handle,
		      struct efi_loaded_image *loaded_image,
		      enum filetype filetype)
{
	efi_status_t efiret;
	const char *options;
	bool is_driver;

	is_driver = (loaded_image->image_code_type == EFI_BOOT_SERVICES_CODE) ||
		(loaded_image->image_code_type == EFI_RUNTIME_SERVICES_CODE);

	if (is_linux_image(filetype, loaded_image->image_base)) {
		pr_debug("Linux kernel detected. Adding bootargs.");
		options = linux_bootargs_get();
		pr_info("add linux options '%s'\n", options);
		if (options) {
			loaded_image->load_options = xstrdup_char_to_wchar(options);
			loaded_image->load_options_size =
				(strlen(options) + 1) * sizeof(wchar_t);
		}
		shutdown_barebox();
	}

	efi_pause_devices();

	efiret = BS->start_image(handle, NULL, NULL);
	if (EFI_ERROR(efiret))
		pr_err("failed to StartImage: %s\n", efi_strerror(efiret));

	efi_continue_devices();

	if (!is_driver)
		BS->unload_image(handle);

	efi_connect_all();
	efi_register_devices();

	return -efi_errno(efiret);
}

static int efi_execute(struct binfmt_hook *b, char *file, int argc, char **argv)
{
	struct efi_loaded_image *loaded_image;
	efi_handle_t handle;
	int ret;

	ret = efi_load_image(file, &loaded_image, &handle);
	if (ret)
		return ret;

	return efi_execute_image(handle, loaded_image, b->type);
}

static struct binfmt_hook binfmt_efi_hook = {
	.type = filetype_exe,
	.hook = efi_execute,
};

static struct image_handler non_efi_handle_linux_x86;

static int do_bootm_nonefi(struct image_data *data)
{
	/* On x86, Linux kernel images have a bzImage header as well as
	 * a PE magic if they're EFI-stubbed.
	 * We have separate file types for x86 Linux images with and
	 * without PE.
	 */
	pr_err("'%s' unsupported: CONFIG_EFI_STUB must be enabled in your Linux kernel config\n",
	       non_efi_handle_linux_x86.name);
	return -ENOSYS;
}

static struct image_handler non_efi_handle_linux_x86 = {
	.name = "non-EFI x86 Linux Image",
	.bootm = do_bootm_nonefi,
	.filetype = filetype_x86_linux_image,
};

static struct binfmt_hook binfmt_arm64_efi_hook = {
	.type = filetype_arm64_efi_linux_image,
	.hook = efi_execute,
};

static struct binfmt_hook binfmt_x86_efi_hook = {
	.type = filetype_x86_efi_linux_image,
	.hook = efi_execute,
};

static int efi_register_image_handler(void)
{
	binfmt_register(&binfmt_efi_hook);

	if (IS_ENABLED(CONFIG_X86)) {
		register_image_handler(&non_efi_handle_linux_x86);
		binfmt_register(&binfmt_x86_efi_hook);
	}

	if (IS_ENABLED(CONFIG_ARM64))
		binfmt_register(&binfmt_arm64_efi_hook);

	return 0;
}
late_efi_initcall(efi_register_image_handler);

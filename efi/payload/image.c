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
#include <linux/ktime.h>
#include <memory.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <bootm.h>
#include <fs.h>
#include <libfile.h>
#include <binfmt.h>
#include <wchar.h>
#include <efi/error.h>
#include <efi/payload.h>
#include <efi/payload/driver.h>
#include <efi/variable.h>

#include "image.h"

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
	if (ret < 0) {
		BS->free_pages(mem, DIV_ROUND_UP(s.st_size, EFI_PAGE_SIZE));
		return NULL;
	}

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

int efi_execute_image(efi_handle_t handle,
		      struct efi_loaded_image *loaded_image,
		      bool is_bootm,
		      enum filetype filetype, const char *options)
{
	efi_status_t efiret;
	wchar_t *load_options = NULL;
	bool is_driver;
	bool no_return;

	is_driver = (loaded_image->image_code_type == EFI_BOOT_SERVICES_CODE) ||
		(loaded_image->image_code_type == EFI_RUNTIME_SERVICES_CODE);
 
	efi_export_dtb();

	/*
	 * A bootm handler is the point of no return, but an EFI-stubbed kernel
	 * started from the shell takes over the machine just the same, so
	 * barebox needs to be shut down in both cases.
	 */
	no_return = is_bootm || filetype_is_linux_efi_image(filetype);

	if (options && *options) {
		load_options = xstrdup_char_to_wchar(options);
		loaded_image->load_options = load_options;
		loaded_image->load_options_size =
			(strlen(options) + 1) * sizeof(wchar_t);
	}

	printf("Booting %s", filetype_is_linux_efi_image(filetype) ?
	       "kernel via StartImage" : "EFI image");
	if (load_options)
		printf(" with options '%s'", options);
	printf("...\n");

	if (no_return) {
		efi_set_variable_usec("LoaderTimeExecUSec", &efi_systemd_vendor_guid,
				      ktime_to_us(ktime_get()));
		shutdown_barebox();
	}

	efi_pause_devices();

	efiret = BS->start_image(handle, NULL, NULL);

	efi_continue_devices();

	if (no_return) {
		pr_emerg("Boot image has unexpectedly returned\n");
		BS->exit(efi_parent_image, efiret, 0, NULL);
	}

	if (EFI_ERROR(efiret))
		pr_err("failed to StartImage: %s\n", efi_strerror(efiret));

	/*
	 * load_options is freed below, so a driver that started successfully
	 * and thus stays loaded must drop its reference to it. The firmware
	 * already unloaded an application that returned and a driver that
	 * failed to start, so their loaded image protocol must not be touched.
	 * Unloading an application that already returned just fails, but is
	 * still needed when StartImage failed before running it.
	 */
	if (is_driver) {
		if (!EFI_ERROR(efiret)) {
			loaded_image->load_options = NULL;
			loaded_image->load_options_size = 0;
		}
	} else {
		BS->unload_image(handle);
	}

	free(load_options);

	efi_connect_all();
	efi_register_devices();

	return -efi_errno(efiret);
}

static int efi_execute(struct binfmt_hook *b, char *file, int argc, char **argv)
{
	struct efi_loaded_image *loaded_image;
	efi_handle_t handle;
	char *options;
	int ret;

	ret = efi_load_image(file, &loaded_image, &handle);
	if (ret)
		return ret;

	/* argv[0] is the image itself, the rest become the load options */
	options = strjoin(" ", &argv[1], argc - 1);

	ret = efi_execute_image(handle, loaded_image, false, b->type, options);

	free(options);

	return ret;
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

static struct binfmt_hook binfmt_zboot_efi_hook = {
	.type = filetype_zboot_efi_linux_image,
	.hook = efi_execute,
};

static int efi_register_image_handler(void)
{
	binfmt_register(&binfmt_efi_hook);
	binfmt_register(&binfmt_zboot_efi_hook);

	if (IS_ENABLED(CONFIG_X86)) {
		register_image_handler(&non_efi_handle_linux_x86);
		binfmt_register(&binfmt_x86_efi_hook);
	}

	if (IS_ENABLED(CONFIG_ARM64))
		binfmt_register(&binfmt_arm64_efi_hook);

	return 0;
}
late_efi_initcall(efi_register_image_handler);

// SPDX-License-Identifier: GPL-2.0-only
/*
 * image.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#define pr_fmt(fmt) "efi-bootm: " fmt

#include <clock.h>
#include <common.h>
#include <globalvar.h>
#include <linux/sizes.h>
#include <linux/ktime.h>
#include <memory.h>
#include <command.h>
#include <magicvar.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <boot.h>
#include <bootm.h>
#include <fs.h>
#include <libfile.h>
#include <binfmt.h>
#include <wchar.h>
#include <efi/payload.h>
#include <efi/payload/driver.h>
#include <efi/error.h>
#include <efi/initrd.h>

#include "image.h"

static int efi_load_os(struct image_data *data,
		       struct efi_loaded_image **loaded_image,
		       efi_handle_t *handle)
{
	efi_status_t efiret;
	efi_handle_t h;
	const void *view;
	size_t size;

	view = loadable_view(data->os, &size) ?: ERR_PTR(-ENODATA);
	if (IS_ERR(view)) {
		pr_err("could not view kernel: %pe\n", view);
		return PTR_ERR(view);
	}

	efiret = BS->load_image(false, efi_parent_image, efi_device_path,
				(void *)view, size, &h);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to LoadImage: %s\n", efi_strerror(efiret));
		goto out_view_free;
	};

	efiret = BS->open_protocol(h, &efi_loaded_image_protocol_guid,
				   (void **)loaded_image, efi_parent_image,
				   NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to OpenProtocol: %s\n", efi_strerror(efiret));
		goto out_unload;
	}

	*handle = h;

	loadable_view_free(data->os, view, size);
	return 0;

out_unload:
	BS->unload_image(h);
out_view_free:
	loadable_view_free(data->os, view, size);
	return -efi_errno(efiret);
}

static int efi_load_ramdisk(struct image_data *data,
			    const void **initrd,
			    size_t *initrd_size)
{
	const void *initrd_mem;
	int ret;

	if (!data->initrd)
		return 0;

	initrd_mem = loadable_view(data->initrd, initrd_size);
	if (IS_ERR(initrd_mem)) {
		pr_err("Cannot open ramdisk image: %pe\n", initrd_mem);
		return PTR_ERR(initrd_mem);
	}

	ret = efi_initrd_register(initrd_mem, *initrd_size);
	if (ret) {
		pr_err("Failed to register initrd: %pe\n", ERR_PTR(ret));
		goto free_mem;
	}

	*initrd = initrd_mem;

	return 0;

free_mem:
	loadable_view_free(data->initrd, initrd_mem, *initrd_size);

	return ret;
}

static int efi_load_fdt(struct image_data *data, void **fdt)
{
	efi_physical_addr_t mem;
	efi_status_t efiret;
	void *vmem;
	size_t bufsize = DIV_ROUND_UP(SZ_2M, EFI_PAGE_SIZE);
	ssize_t ret;

	if (!data->oftree)
		return 0;

	efiret = BS->allocate_pages(EFI_ALLOCATE_ANY_PAGES, EFI_ACPI_RECLAIM_MEMORY,
				    bufsize, &mem);
	if (EFI_ERROR(efiret)) {
		pr_err("Failed to allocate pages for FDT: %s\n", efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	vmem = efi_phys_to_virt(mem);

	ret = loadable_extract_into_buf_full(data->oftree, vmem,
					     bufsize * EFI_PAGE_SIZE);
	if (ret < 0)
		goto free_efi_mem;

	efiret = BS->install_configuration_table(&efi_fdt_guid, vmem);
	if (EFI_ERROR(efiret)) {
		pr_err("Failed to install FDT: %s\n", efi_strerror(efiret));
		ret = -efi_errno(efiret);
		goto free_efi_mem;
	}

	*fdt = vmem;
	return 0;

free_efi_mem:
	BS->free_pages(mem, bufsize);
	return ret;
}

static void efi_unload_fdt(void *fdt)
{
	if (!fdt)
		return;

	BS->install_configuration_table(&efi_fdt_guid, NULL);
	BS->free_pages(efi_virt_to_phys(fdt), DIV_ROUND_UP(SZ_2M, EFI_PAGE_SIZE));
}

static int do_bootm_efi_stub(struct image_data *data)
{
	struct efi_loaded_image *loaded_image;
	void *fdt = NULL;
	const void *initrd = NULL;
	size_t initrd_size;
	efi_handle_t handle = NULL; /* silence compiler warning */
	enum filetype type;
	int ret;

	ret = efi_load_os(data, &loaded_image, &handle);
	if (ret)
		return ret;

	ret = efi_load_fdt(data, &fdt);
	if (ret)
		goto unload_os;

	ret = efi_load_ramdisk(data, &initrd, &initrd_size);
	if (ret)
		goto unload_oftree;

	type = file_detect_type(loaded_image->image_base, PAGE_SIZE);

	if (data->dryrun)
		goto unload_ramdisk;

	ret = efi_execute_image(handle, loaded_image, type);
unload_ramdisk:
	if (initrd) {
		efi_initrd_unregister();
		loadable_view_free(data->initrd, initrd, initrd_size);
	}
unload_oftree:
	efi_unload_fdt(fdt);
unload_os:
	BS->unload_image(handle);

	return ret;
}

static int efi_app_execute(struct image_data *data)
{
	struct efi_loaded_image *loaded_image;
	efi_handle_t handle;
	enum filetype type;
	int ret;

	ret = efi_load_image(data->os_file, &loaded_image, &handle);
	if (ret)
		return ret;

	type = file_detect_type(loaded_image->image_base, PAGE_SIZE);

	return efi_execute_image(handle, loaded_image, type);
}

static int linux_efi_handover = true;

bool efi_x86_boot_method_check(struct image_handler *handler,
			       struct image_data *data,
			       enum filetype detected_filetype)
{
	if (handler->filetype != detected_filetype)
		return false;

	if (IS_ENABLED(CONFIG_EFI_HANDOVER_PROTOCOL) && linux_efi_handover)
		return handler == &efi_x86_linux_handle_handover;
	else
		return handler == &efi_x86_linux_handle_tr;
}

static struct image_handler efi_app_handle_tr = {
	.name = "EFI Application",
	.bootm = efi_app_execute,
	.filetype = filetype_exe,
};

struct image_handler efi_x86_linux_handle_tr = {
	.name = "EFI X86 Linux kernel (StartImage)",
	.bootm = do_bootm_efi_stub,
	.filetype = filetype_x86_efi_linux_image,
	.check_image = efi_x86_boot_method_check,
};

static struct image_handler efi_arm64_handle_tr = {
	.name = "EFI ARM64 Linux kernel",
	.bootm = do_bootm_efi_stub,
	.check_image = bootm_efi_check_image,
	.filetype = filetype_arm64_efi_linux_image,
};

BAREBOX_MAGICVAR(global.linux.efi.handover,
		 "Use legacy x86 handover protocol instead of StartImage BootService");

static int efi_register_bootm_handler(void)
{
	register_image_handler(&efi_app_handle_tr);

	if (IS_ENABLED(CONFIG_X86)) {
		if (IS_ENABLED(CONFIG_EFI_HANDOVER_PROTOCOL))
			globalvar_add_simple_bool("linux.efi.handover", &linux_efi_handover);
		register_image_handler(&efi_x86_linux_handle_tr);
	}

	if (IS_ENABLED(CONFIG_ARM64))
		register_image_handler(&efi_arm64_handle_tr);

	return 0;
}
late_efi_initcall(efi_register_bootm_handler);

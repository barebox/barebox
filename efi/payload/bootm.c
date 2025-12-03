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
#include <efi.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <boot.h>
#include <bootm.h>
#include <fs.h>
#include <libfile.h>
#include <binfmt.h>
#include <wchar.h>
#include <image-fit.h>
#include <efi/efi-payload.h>
#include <efi/efi-device.h>

#include "image.h"

static bool ramdisk_is_fit(struct image_data *data)
{
	struct stat st;

	if (!IS_ENABLED(CONFIG_BOOTM_FITIMAGE))
		return false;

	if (bootm_signed_images_are_forced())
		return true;

	if (data->initrd_file) {
		if (!stat(data->initrd_file, &st) && st.st_size > 0)
			return false;
	}

	return data->os_fit ? fit_has_image(data->os_fit,
			data->fit_config, "ramdisk") > 0 : false;
}

static bool fdt_is_fit(struct image_data *data)
{
	struct stat st;

	if (!IS_ENABLED(CONFIG_BOOTM_FITIMAGE))
		return false;

	if (bootm_signed_images_are_forced())
		return true;

	if (data->oftree_file) {
		if (!stat(data->oftree_file, &st) && st.st_size > 0)
			return false;
	}

	return data->os_fit ? fit_has_image(data->os_fit,
			data->fit_config, "fdt") > 0 : false;
}

static bool os_is_fit(struct image_data *data)
{
	if (!IS_ENABLED(CONFIG_BOOTM_FITIMAGE))
		return false;

	if (bootm_signed_images_are_forced())
		return true;

	return data->os_fit;
}

static int efi_load_os(struct image_data *data,
		       struct efi_loaded_image **loaded_image,
		       efi_handle_t *handle)
{
	efi_status_t efiret;
	efi_handle_t h;

	if (!os_is_fit(data))
		return efi_load_image(data->os_file, loaded_image, handle);

	if (!data->fit_kernel)
		return -ENOENT;

	efiret = BS->load_image(false, efi_parent_image, efi_device_path,
				(void *)data->fit_kernel, data->fit_kernel_size, &h);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to LoadImage: %s\n", efi_strerror(efiret));
		goto out_mem;
	};

	efiret = BS->open_protocol(h, &efi_loaded_image_protocol_guid,
				   (void **)loaded_image, efi_parent_image,
				   NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to OpenProtocol: %s\n", efi_strerror(efiret));
		goto out_unload;
	}

	*handle = h;

	return 0;

out_unload:
	BS->unload_image(h);
out_mem:
	return -efi_errno(efiret);
}

static int efi_load_ramdisk(struct image_data *data, void **initrd)
{
	unsigned long initrd_size;
	void *initrd_mem;
	int ret;

	if (ramdisk_is_fit(data)) {
		ret = fit_open_image(data->os_fit, data->fit_config, "ramdisk",
				     (const void **)&initrd_mem, &initrd_size);
		if (ret) {
			pr_err("Cannot open ramdisk image in FIT image: %m\n");
			return ret;
		}
	} else {
		if (!data->initrd_file)
			return 0;

		pr_info("Loading ramdisk from '%s'\n", data->initrd_file);

		initrd_mem = read_file(data->initrd_file, &initrd_size);
		if (!initrd_mem) {
			ret = -errno;
			pr_err("Failed to read initrd from file '%s': %m\n",
			       data->initrd_file);
			return ret;
		}
	}

	ret = efi_initrd_register(initrd_mem, initrd_size);
	if (ret) {
		pr_err("Failed to register initrd: %pe\n", ERR_PTR(ret));
		goto free_mem;
	}

	*initrd = initrd_mem;

	return 0;

free_mem:
	free(initrd);

	return ret;
}

static int efi_load_fdt(struct image_data *data, void **fdt)
{
	efi_physical_addr_t mem;
	efi_status_t efiret;
	void *of_tree, *vmem;
	unsigned long of_size;
	int ret;

	if (fdt_is_fit(data)) {
		ret = fit_open_image(data->os_fit, data->fit_config, "fdt",
				     (const void **)&of_tree, &of_size);
		if (ret) {
			pr_err("Cannot open FDT image in FIT image: %m\n");
			return ret;
		}
	} else {
		if (!data->oftree_file)
			return 0;

		pr_info("Loading devicetree from '%s'\n", data->oftree_file);

		of_tree = read_file(data->oftree_file, &of_size);
		if (!of_tree) {
			ret = -errno;
			pr_err("Failed to read oftree: %m\n");
			return ret;
		}
	}

	efiret = BS->allocate_pages(EFI_ALLOCATE_ANY_PAGES,
				    EFI_ACPI_RECLAIM_MEMORY,
				    DIV_ROUND_UP(SZ_2M, EFI_PAGE_SIZE), &mem);
	if (EFI_ERROR(efiret)) {
		pr_err("Failed to allocate pages for FDT: %s\n", efi_strerror(efiret));
		goto free_mem;
	}

	vmem = efi_phys_to_virt(mem);
	memcpy(vmem, of_tree, of_size);

	efiret = BS->install_configuration_table(&efi_fdt_guid, vmem);
	if (EFI_ERROR(efiret)) {
		pr_err("Failed to install FDT: %s\n", efi_strerror(efiret));
		goto free_efi_mem;
	}

	*fdt = vmem;
	return 0;

free_efi_mem:
	BS->free_pages(mem, DIV_ROUND_UP(SZ_2M, EFI_PAGE_SIZE));
free_mem:
	free(of_tree);
	return -efi_errno(efiret);
}

static void efi_unload_fdt(void *fdt)
{
	if (!fdt)
		return;

	BS->install_configuration_table(&efi_fdt_guid, NULL);
	BS->free_pages(efi_virt_to_phys(fdt), SZ_2M);
}

static int do_bootm_efi_stub(struct image_data *data)
{
	struct efi_loaded_image *loaded_image;
	void *fdt = NULL, *initrd = NULL;
	efi_handle_t handle;
	enum filetype type;
	int ret;

	ret = efi_load_os(data, &loaded_image, &handle);
	if (ret)
		return ret;

	ret = efi_load_fdt(data, &fdt);
	if (ret)
		goto unload_os;

	ret = efi_load_ramdisk(data, &initrd);
	if (ret)
		goto unload_oftree;

	type = file_detect_type(loaded_image->image_base, PAGE_SIZE);
	ret = efi_execute_image(handle, loaded_image, type);
	if (ret)
		goto unload_ramdisk;

	return 0;

unload_ramdisk:
	if (initrd)
		efi_initrd_unregister();
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

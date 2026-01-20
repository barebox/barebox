// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/162a6b3df349295bf766c1d128d71b4547e8d56e/lib/efi_loader/efi_helper.c
/*
 * Copyright (c) 2016 Alexander Graf
 */
#define pr_fmt(fmt) "efi-loader: bootm: " fmt

#include <clock.h>
#include <linux/sizes.h>
#include <memory.h>
#include <command.h>
#include <magicvar.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <bootargs.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <fs.h>
#include <filetype.h>
#include <libfile.h>
#include <bootm.h>
#include <wchar.h>
#include <efi/mode.h>
#include <efi/loader.h>
#include <efi/loader/object.h>
#include <efi/loader/option.h>
#include <efi/loader/table.h>
#include <efi/loader/devicepath.h>
#include <efi/loader/image.h>
#include <efi/loader/event.h>
#include <efi/guid.h>
#include <efi/services.h>
#include <efi/error.h>
#include <efi/initrd.h>
#include <efi/devicepath.h>

/**
 * copy_fdt() - Copy the device tree to a new location available to EFI
 *
 * The FDT is copied to a suitable location within the EFI memory map.
 * Additional 12 KiB are added to the space in case the device tree needs to be
 * expanded later with fdt_open_into().
 *
 * @fdtp:	On entry a pointer to the flattened device tree.
 *		On exit a pointer to the copy of the flattened device tree.
 *		FDT start
 * Return:	status code
 */
static efi_status_t copy_fdt(void **fdtp)
{
	efi_status_t efiret;
	void *fdt, *new_fdt;
	static u64 new_fdt_addr;
	static efi_uintn_t fdt_pages;
	size_t fdt_size;

	/*
	 * Remove the configuration table that might already be
	 * installed, ignoring EFI_NOT_FOUND if no device-tree
	 * is installed
	 */
	efi_install_configuration_table(&efi_fdt_guid, NULL);

	if (new_fdt_addr) {
		pr_debug("%s: Found allocated memory at %#llx, with %#zx pages\n",
			  __func__, new_fdt_addr, fdt_pages);

		efiret = efi_free_pages(new_fdt_addr, fdt_pages);
		if (efiret != EFI_SUCCESS)
			pr_err("Unable to free up existing FDT memory region\n");

		new_fdt_addr = 0;
		fdt_pages = 0;
	}

	/*
	 * Give us at least 12 KiB of breathing room in case the device tree
	 * needs to be expanded later.
	 */
	fdt = *fdtp;
	fdt_pages = efi_size_in_pages(fdt_totalsize(fdt) + 0x3000);
	fdt_size = fdt_pages << EFI_PAGE_SHIFT;

	/*
	 * Safe fdt location is at 127 MiB.
	 * On the sandbox convert from the sandbox address space.
	 */
	efiret = efi_allocate_pages(EFI_ALLOCATE_ANY_PAGES,
				 EFI_ACPI_RECLAIM_MEMORY, fdt_pages,
				 &new_fdt_addr, "FDT");
	if (efiret != EFI_SUCCESS) {
		pr_err("Failed to reserve space for FDT\n");
		goto done;
	}

	new_fdt = (void *)(uintptr_t)new_fdt_addr;
	memcpy(new_fdt, fdt, fdt_totalsize(fdt));
	fdt_set_totalsize(new_fdt, fdt_size);

	*fdtp = new_fdt;

done:
	return efiret;
}

/**
 * efi_install_fdt() - install device tree
 *
 * If fdt is not NULL, the device tree located at that memory
 * address will will be installed as configuration table, otherwise the device
 * tree located at the address indicated by environment variable fdt_addr or as
 * fallback fdtcontroladdr will be used.
 *
 * On architectures using ACPI tables device trees shall not be installed as
 * configuration table.
 *
 * @fdt:	address of device tree
 *		the hardware device tree as indicated by environment variable
 *		fdt_addr or as fallback the internal device tree as indicated by
 *		the environment variable fdtcontroladdr
 * Return:	status code
 */
static efi_status_t efi_install_fdt(void *fdt)
{
	const struct fdt_header *hdr = fdt;
	/*
	 * The EBBR spec requires that we have either an FDT or an ACPI table
	 * but not both.
	 */
	efi_status_t ret;

	if (hdr->magic != cpu_to_fdt32(FDT_MAGIC)) {
		pr_err("bad magic: 0x%08x\n", fdt32_to_cpu(hdr->magic));
		return EFI_LOAD_ERROR;
	}

	if (hdr->version != cpu_to_fdt32(17)) {
		pr_err("bad dt version: 0x%08x\n", fdt32_to_cpu(hdr->version));
		return EFI_LOAD_ERROR;
	}

	/* Prepare device tree for payload */
	ret = copy_fdt(&fdt);
	if (ret) {
		pr_err("out of memory\n");
		return EFI_OUT_OF_RESOURCES;
	}

	/* Install device tree as UEFI table */
	ret = efi_install_configuration_table(&efi_fdt_guid, fdt);
	if (ret != EFI_SUCCESS) {
		pr_err("failed to install device tree\n");
		return ret;
	}

	return EFI_SUCCESS;
}

/**
 * efi_install_initrd() - install initrd
 *
 * Install the initrd located at @initrd using the EFI_LOAD_FILE2
 * protocol.
 *
 * @initrd:	address of initrd or NULL if none is provided
 * @initrd_sz:	size of initrd
 * Return:	status code
 */
static efi_status_t efi_install_initrd(struct image_data *data,
				       struct resource *source)
{
	const struct resource *initrd_res;
	unsigned long initrd_start;

	if (!IS_ENABLED(CONFIG_BOOTM_INITRD))
		return EFI_SUCCESS;

	if (UIMAGE_IS_ADDRESS_VALID(data->initrd_address))
		initrd_start = data->initrd_address;
	else
		initrd_start = EFI_PAGE_ALIGN(source->end + 1);

	initrd_res = bootm_load_initrd(data, initrd_start);
	if (IS_ERR(initrd_res))
		return PTR_ERR(initrd_res);
	if (initrd_res)
		efi_initrd_register((void *)initrd_res->start,
				    resource_size(initrd_res));

	return EFI_SUCCESS;
}

static int efi_loader_bootm(struct image_data *data)
{
	resource_size_t start, end;
	void *load_option = NULL;
	u32 load_option_size = 0;
	efi_handle_t handle;
	struct efi_device_path *file_path = NULL;
	struct resource *source;
	struct efi_event *evt;
	size_t exit_data_size = 0;
	u16 *exit_data = NULL;
	efi_status_t efiret;
	int ret = 0;
	void *fdt;
	int flags = 0;

	memory_bank_first_find_space(&start, &end);
	data->os_address = start;

	source = file_to_sdram(data->os_file, data->os_address, MEMTYPE_LOADER_CODE);
	if (!source)
		return -EINVAL;

	if (filetype_is_linux_efi_image(data->os_type)) {
		const char *options;

		options = linux_bootargs_get();
		if (options) {
			load_option = xstrdup_char_to_wchar(options);
			load_option_size = (strlen(options) + 1) * sizeof(wchar_t);
		}
	}

	file_path = efi_dp_from_file(AT_FDCWD, data->os_file);

	pr_info("Loading %pD\n", file_path);

	/* Initialize EFI drivers */
	efiret = efi_init_obj_list();
	if (efiret) {
		pr_err("Cannot initialize UEFI sub-system: %pe\n",
			ERR_PTR(-efi_errno(ret)));
		goto out;
	}

	ret = -EINVAL;

	fdt = bootm_get_devicetree(data);
	if (IS_ERR(fdt))
		return PTR_ERR(fdt);
	if (fdt) {
		ret = efi_install_fdt(fdt);
		if (ret)
			return ret;
	}

	efiret = efi_install_initrd(data, source);
	if(efiret != EFI_SUCCESS)
		goto out;

	if (data->verbose >= 1)
		flags |= EFI_VERBOSE_RUN;
	if (data->dryrun)
		flags |= EFI_DRYRUN;

	efiret = efiloader_load_image(false, efi_root, file_path,
				(void *)source->start,
				resource_size(source), &handle);
	if (efiret != EFI_SUCCESS) {
		pr_err("Loading image failed\n");
		goto out;
	}

	efiret = efi_set_load_options(handle, load_option_size, load_option);
	if (efiret)
		goto out;

	/* On ARM, PBL should have already moved us into EL2 here, so
	 * no need to switch modes
	 */

	/*
	 * The UEFI standard requires that the watchdog timer is set to five
	 * minutes when invoking an EFI boot option.
	 *
	 * Unified Extensible Firmware Interface (UEFI), version 2.7 Errata A
	 * 7.5. Miscellaneous Boot Services - EFI_BOOT_SERVICES.SetWatchdogTimer
	 */
	ret = efi_set_watchdog(300);
	if (ret != EFI_SUCCESS) {
		pr_err("failed to set watchdog timer\n");
		goto out;
	}

	/* Call our payload! */
	ret = __efi_start_image(handle, &exit_data_size, &exit_data, flags);
	if (ret != EFI_SUCCESS) {
		pr_err("## Application failed, r = %lu\n",
			ret & ~EFI_ERROR_MASK);
		if (exit_data) {
			pr_err("## %ls\n", exit_data);
			efi_free_pool(exit_data);
		}
	}

	/* Notify EFI_EVENT_GROUP_RETURN_TO_EFIBOOTMGR event group. */
	list_for_each_entry(evt, &efi_events, link) {
		if (evt->group &&
		    !efi_guidcmp(*evt->group,
				 efi_guid_event_group_return_to_efibootmgr)) {
			efi_signal_event(evt);
			systab.boottime->close_event(evt);
			break;
		}
	}

	/* Control is returned to us, disable EFI watchdog */
	efi_set_watchdog(0);

	return ret;

out:
	efi_initrd_unregister();
	efi_install_configuration_table(&efi_fdt_guid, NULL);
	efi_free_pool(file_path);
	free(load_option);
	release_sdram_region(source);

	return ret ?: -efi_errno(efiret);
}

static struct image_handler riscv_linux_efi_handler = {
	.name = "RISC-V Linux/EFI image",
	.bootm = efi_loader_bootm,
	.check_image = bootm_efi_check_image,
	.filetype = filetype_riscv_efi_linux_image,
};

static struct image_handler aarch64_linux_efi_handler = {
	.name = "ARM aarch64 Linux/EFI image",
	.bootm = efi_loader_bootm,
	.check_image = bootm_efi_check_image,
	.filetype = filetype_arm64_efi_linux_image,
};

static struct image_handler arm32_linux_efi_handler = {
	.name = "ARM arm32 Linux/EFI image",
	.bootm = efi_loader_bootm,
	.check_image = bootm_efi_check_image,
	.filetype = filetype_arm_efi_zimage,
};

static struct image_handler efi_image_loader = {
	.name = "EFI Application",
	.bootm = efi_loader_bootm,
	.filetype = filetype_exe,
};

static int efiloader_bootm_register(void)
{
	if (IS_ENABLED(CONFIG_ARM32))
		register_image_handler(&arm32_linux_efi_handler);
	if (IS_ENABLED(CONFIG_ARM64))
		register_image_handler(&aarch64_linux_efi_handler);
	if (IS_ENABLED(CONFIG_RISCV))
		register_image_handler(&riscv_linux_efi_handler);
	return register_image_handler(&efi_image_loader);
}
late_initcall(efiloader_bootm_register);

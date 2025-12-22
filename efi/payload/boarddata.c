// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "efi-boarddata: " fmt

#ifdef CONFIG_DEBUG_INITCALLS
#define DEBUG
#endif

#include <efi/payload.h>
#include <efi/error.h>
#include <efi/guid.h>
#include <memory.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <debug_ll.h>
#include <init.h>
#include <pbl/handoff-data.h>

static int handle_efi_boarddata(void)
{
	size_t size;
	struct barebox_efi_data *efidata;
	efi_status_t efiret;

	efidata = handoff_data_get_entry(HANDOFF_DATA_EFI, &size);
	if (!efidata)
		return 0;

	barebox_add_memory_bank("ram0", mem_malloc_start(), mem_malloc_size());

	efi_parent_image = efidata->image;
	efi_sys_table = efidata->sys_table;
	BS = efi_sys_table->boottime;
	RT = efi_sys_table->runtime;

	pr_debug("parent_image = %p, sys_table = %p\n",
		 efi_parent_image, efi_sys_table);

	efiret = BS->open_protocol(efi_parent_image, &efi_loaded_image_protocol_guid,
			(void **)&efi_loaded_image,
			efi_parent_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (!EFI_ERROR(efiret))
		BS->handle_protocol(efi_loaded_image->device_handle,
				&efi_device_path_protocol_guid, (void **)&efi_device_path);

	return 0;
}
pure_initcall(handle_efi_boarddata);

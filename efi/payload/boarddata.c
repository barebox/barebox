// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "efi-boarddata: " fmt

#ifdef CONFIG_DEBUG_INITCALLS
#define DEBUG
#endif

#include <efi/efi-payload.h>
#include <efi.h>
#include <boarddata.h>
#include <memory.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <debug_ll.h>
#include <init.h>

static int handle_efi_boarddata(void)
{
	const struct barebox_boarddata *bd = barebox_get_boarddata();
	efi_status_t efiret;

	if (!barebox_boarddata_is_machine(bd, BAREBOX_MACH_TYPE_EFI))
		return 0;

	barebox_add_memory_bank("ram0", mem_malloc_start(), mem_malloc_size());

	efi_parent_image = bd->image;
	efi_sys_table = bd->sys_table;
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

// SPDX-License-Identifier: GPL-2.0-only

#ifdef CONFIG_DEBUG_LL
#define DEBUG
#endif

#include <linux/kernel.h>
#include <linux/sizes.h>
#include <efi.h>
#include <efi/efi-payload.h>
#include <memory.h>
#include <common.h>

/**
 * efi-main - Entry point for EFI images
 */
void efi_main(efi_handle_t image, struct efi_system_table *sys_table)
{
	efi_status_t efiret;
	void *mem;

#ifdef DEBUG
	sys_table->con_out->output_string(sys_table->con_out, L"barebox\n");
#endif

	BS = sys_table->boottime;

	efi_parent_image = image;
	efi_sys_table = sys_table;
	RT = sys_table->runtime;

	efiret = BS->open_protocol(efi_parent_image, &efi_loaded_image_protocol_guid,
			(void **)&efi_loaded_image,
			efi_parent_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (!EFI_ERROR(efiret))
		BS->handle_protocol(efi_loaded_image->device_handle,
				&efi_device_path_protocol_guid, (void **)&efi_device_path);

	mem = efi_earlymem_alloc(sys_table, SZ_16M, EFI_BOOT_SERVICES_DATA);

	mem_malloc_init(mem, mem + SZ_16M - 1);

	start_barebox();
}

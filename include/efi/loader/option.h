/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __EFI_LOADER_OPTION_H__
#define __EFI_LOADER_OPTION_H__

#include <efi/types.h>

/*
 * See section 3.1.3 in the v2.7 UEFI spec for more details on
 * the layout of EFI_LOAD_OPTION.  In short it is:
 *
 *    typedef struct _EFI_LOAD_OPTION {
 *        UINT32 Attributes;
 *        UINT16 FilePathListLength;
 *        // CHAR16 Description[];   <-- variable length, NULL terminated
 *        // EFI_DEVICE_PATH_PROTOCOL FilePathList[];
 *						 <-- FilePathListLength bytes
 *        // UINT8 OptionalData[];
 *    } EFI_LOAD_OPTION;
 */
struct efi_load_option_unpacked {
	u32 attributes;
	u16 file_path_length;
	u16 *label;
	struct efi_device_path *file_path;
	const u8 *optional_data;
};

efi_status_t efi_deserialize_load_option(struct efi_load_option_unpacked *lo,
					 u8 *data, efi_uintn_t *size);
size_t efi_serialize_load_option(struct efi_load_option_unpacked *lo,
				 u8 **data);
efi_status_t efi_set_load_options(efi_handle_t handle,
				  efi_uintn_t load_options_size,
				  void *load_options);

#endif

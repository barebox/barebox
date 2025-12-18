/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _EFI_LOADER_IMAGE_H
#define _EFI_LOADER_IMAGE_H

#include <efi/types.h>

#define EFI_DRYRUN		1
#define EFI_VERBOSE_RUN		2

/* Load image */
efi_status_t EFIAPI efiloader_load_image(bool boot_policy,
					 efi_handle_t parent_image,
					 struct efi_device_path *file_path,
					 void *source_buffer,
					 efi_uintn_t source_size,
					 efi_handle_t *image_handle);

#endif

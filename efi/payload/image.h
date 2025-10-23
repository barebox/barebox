/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_PAYLOAD_IMAGE_H__
#define __EFI_PAYLOAD_IMAGE_H__

#include <efi/types.h>

int efi_load_image(const char *file, struct efi_loaded_image **loaded_image,
		   efi_handle_t *h);

int efi_execute_image(efi_handle_t handle,
		      struct efi_loaded_image *loaded_image,
		      enum filetype filetype);

#endif

/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_PAYLOAD_IMAGE_H__
#define __EFI_PAYLOAD_IMAGE_H__

#include <efi/types.h>
#include <filetype.h>

struct efi_loaded_image;
struct image_data;

int efi_load_image(const char *file, struct efi_loaded_image **loaded_image,
		   efi_handle_t *h);

int efi_execute_image(efi_handle_t handle,
		      struct efi_loaded_image *loaded_image,
		      enum filetype filetype);

extern struct image_handler efi_x86_linux_handle_tr;
extern struct image_handler efi_x86_linux_handle_handover;

bool efi_x86_boot_method_check(struct image_handler *handler,
			       struct image_data *data,
			       enum filetype detected_filetype);

#endif

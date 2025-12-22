/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __EFI_LOADER_PE_H_
#define __EFI_LOADER_PE_H_

#include <efi/types.h>
#include <efi/loader/object.h>
#include <asm/setjmp.h>
#include <asm/setjmp.h>

struct efi_loaded_image;
struct efi_system_table;
struct _WIN_CERTIFICATE;

enum efi_image_auth_status {
	EFI_IMAGE_AUTH_FAILED = 0,
	EFI_IMAGE_AUTH_PASSED,
};

/**
 * struct efi_loaded_image_obj - handle of a loaded image
 *
 * @header:		EFI object header
 * @exit_status:	exit status passed to Exit()
 * @exit_data_size:	exit data size passed to Exit()
 * @exit_data:		exit data passed to Exit()
 * @exit_jmp:		long jump buffer for returning from started image
 * @entry:		entry address of the relocated image
 * @image_type:		indicates if the image is an applicition or a driver
 * @auth_status:	indicates if the image is authenticated
 */
struct efi_loaded_image_obj {
	struct efi_object header;
	efi_status_t *exit_status;
	efi_uintn_t *exit_data_size;
	u16 **exit_data;
	struct jmp_buf_data *exit_jmp;
	EFIAPI efi_status_t (*entry)(efi_handle_t image_handle,
				     struct efi_system_table *st);
	u16 image_type;
	enum efi_image_auth_status auth_status;
};

/* A part of an image, used for hashing */
struct image_region {
	const void *data;
	int size;
};

/**
 * struct efi_image_regions - A list of memory regions
 *
 * @max:	Maximum number of regions
 * @num:	Number of regions
 * @reg:	array of regions
 */
struct efi_image_regions {
	int			max;
	int			num;
	struct image_region	reg[];
};

struct efi_system_table;

/* Print information about all loaded images */
void efi_print_image_infos(void *pc);

efi_status_t efi_image_region_add(struct efi_image_regions *regs,
				  const void *start, const void *end,
				  int nocheck);

void *efi_prepare_aligned_image(void *efi, u64 *efi_size);

bool efi_image_parse(void *efi, size_t len, struct efi_image_regions **regp,
		     struct _WIN_CERTIFICATE **auth, size_t *auth_len);

/* Check if a buffer contains a PE-COFF image */
efi_status_t efi_check_pe(void *buffer, size_t size, void **nt_header);
/* PE loader implementation */
efi_status_t efi_load_pe(struct efi_loaded_image_obj *handle,
			 void *efi, size_t efi_size,
			 struct efi_loaded_image *loaded_image_info);

#endif

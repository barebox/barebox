/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-SnippetCopyrightText: 2016 Alexander Graf */

#ifndef _EFI_LOADER_DEBUG_H
#define _EFI_LOADER_DEBUG_H

#include <efi/types.h>

struct efi_loaded_image;

/* Called by bootefi to initialize debug */
efi_status_t efi_initialize_system_table_pointer(void);
/* Called by efi_load_image for register debug info */
efi_status_t efi_core_new_debug_image_info_entry(u32 image_info_type,
						 struct efi_loaded_image *loaded_image,
						 efi_handle_t image_handle);
void efi_core_remove_debug_image_info_entry(efi_handle_t image_handle);

#define EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS 0x01
#define EFI_DEBUG_IMAGE_INFO_TABLE_MODIFIED     0x02

#define EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL  0x01

/**
 * struct efi_debug_image_info_normal - Store Debug Information for normal
 * image.
 * @image_info_type: the type of image info.
 * @loaded_image_protocol_instance: the pointer to struct efi_loaded_image.
 * @image_handle: the EFI handle of the image.
 *
 * This struct is created by efi_load_image() and store the information
 * for debugging an normal image.
 */
struct efi_debug_image_info_normal {
	u32 image_info_type;
	struct efi_loaded_image *loaded_image_protocol_instance;
	efi_handle_t image_handle;
};

/**
 * union efi_debug_image_info - The union to store a pointer for EFI
 * DEBUG IMAGE INFO.
 * @image_info_type: the type of the image_info if it is not a normal image.
 * @normal_image: The pointer to a normal image.
 *
 * This union is for a pointer that can point to the struct of normal_image.
 * Or it points to an image_info_type.
 */
union efi_debug_image_info {
	u32 *image_info_type;
	struct efi_debug_image_info_normal *normal_image;
};

/**
 * struct efi_debug_image_info_table_header - store the array of
 * struct efi_debug_image_info.
 * @update_status: Status to notify this struct is ready to use or not.
 * @table_size: The number of elements of efi_debug_image_info_table.
 * @efi_debug_image_info_table: The array of efi_debug_image_info.
 *
 * This struct stores the array of efi_debug_image_info. The
 * number of elements is table_size.
 */
struct efi_debug_image_info_table_header {
	volatile u32 update_status;
	u32 table_size;
	union efi_debug_image_info *efi_debug_image_info_table;
};

/* structure for EFI_DEBUG_SUPPORT_PROTOCOL */
extern struct efi_debug_image_info_table_header efi_m_debug_info_table_header;

#endif

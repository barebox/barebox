/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_PROTOCOL_DEVICE_PATH_H
#define __EFI_PROTOCOL_DEVICE_PATH_H

#include <efi/types.h>

struct efi_device_path_utilities_protocol {
	size_t (EFIAPI *get_device_path_size)(
		const struct efi_device_path *device_path);
	struct efi_device_path *(EFIAPI *duplicate_device_path)(
		const struct efi_device_path *device_path);
	struct efi_device_path *(EFIAPI *append_device_path)(
		const struct efi_device_path *src1,
		const struct efi_device_path *src2);
	struct efi_device_path *(EFIAPI *append_device_node)(
		const struct efi_device_path *device_path,
		const struct efi_device_path *device_node);
	struct efi_device_path *(EFIAPI *append_device_path_instance)(
		const struct efi_device_path *device_path,
		const struct efi_device_path *device_path_instance);
	struct efi_device_path *(EFIAPI *get_next_device_path_instance)(
		struct efi_device_path **device_path_instance,
		size_t *device_path_instance_size);
	bool (EFIAPI *is_device_path_multi_instance)(
		const struct efi_device_path *device_path);
	struct efi_device_path *(EFIAPI *create_device_node)(
		uint8_t node_type,
		uint8_t node_sub_type,
		uint16_t node_length);
};
#endif

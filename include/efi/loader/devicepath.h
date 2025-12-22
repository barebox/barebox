/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_LOADER_DEVICE_PATH_H
#define __EFI_LOADER_DEVICE_PATH_H

#include <efi/types.h>
#include <linux/compiler.h>

struct efi_device_path *efi_dp_next(const struct efi_device_path *dp);

int efi_dp_match(const struct efi_device_path *a,
		 const struct efi_device_path *b);

efi_handle_t efi_dp_find_obj(struct efi_device_path *dp,
			     const efi_guid_t *guid,
			     struct efi_device_path **rem);

size_t efi_dp_instance_size(const struct efi_device_path *dp);

size_t efi_dp_size(const struct efi_device_path *dp);

struct efi_device_path *efi_dp_dup(const struct efi_device_path *dp);

struct efi_device_path *efi_dp_concat(const struct efi_device_path *dp1,
				      const struct efi_device_path *dp2,
				      size_t split_end_node);

struct efi_device_path *efi_dp_append_node(const struct efi_device_path *dp,
					   const struct efi_device_path *node);

struct efi_device_path *efi_dp_create_device_node(const u8 type,
						  const u8 sub_type,
						  const u16 length);
struct efi_device_path *efi_dp_append_instance(
		const struct efi_device_path *dp,
		const struct efi_device_path *dpi);

struct efi_device_path *efi_dp_get_next_instance(struct efi_device_path **dp,
						 size_t *size);

bool efi_dp_is_multi_instance(const struct efi_device_path *dp);

struct efi_device_path *efi_dp_from_file(int dirfd, const char *);

struct cdev;

struct efi_device_path *efi_dp_from_cdev(struct cdev *cdev, bool full);

efi_status_t efi_dp_split_file_path(struct efi_device_path *full_path,
				    struct efi_device_path **device_path,
				    struct efi_device_path **file_path);

struct efi_device_path *efi_dp_from_mem(uint32_t mem_type,
					uint64_t start_address,
					uint64_t end_address);

const struct efi_device_path *efi_dp_last_node(
			const struct efi_device_path *dp);

#define EFI_DP_TYPE(_dp, _type, _subtype) \
	(((_dp)->type == DEVICE_PATH_TYPE_##_type) && \
	 ((_dp)->sub_type == DEVICE_PATH_SUB_TYPE_##_subtype))


char *efi_dp_from_file_tostr(int dirfd, const char *path);

#endif /* __EFI_LOADER_DEVICE_PATH_H */

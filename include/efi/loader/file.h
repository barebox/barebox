/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __EFI_LOADER_FILE_H_
#define __EFI_LOADER_FILE_H_

#include <efi/types.h>

struct cdev;
struct efi_device_path;
struct efi_simple_file_system_protocol;
struct efi_file_handle;

struct efi_simple_file_system_protocol *
efi_fs_from_path(struct efi_device_path *fp);

/* open file from device-path: */
struct efi_file_handle *efi_file_from_path(struct efi_device_path *fp);

efi_status_t efi_file_size(struct efi_file_handle *fh, efi_uintn_t *size);

/**
 * efi_simple_file_system() - create simple file system protocol
 *
 * Create a simple file system protocol for a partition.
 *
 * @cdev:	barebox cdev
 * @dp:		device path
 * Return:	status code
 */
struct efi_simple_file_system_protocol *
efi_simple_file_system(struct cdev *cdev,
		       struct efi_device_path *dp);

#endif

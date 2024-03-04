/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_UTIL_H
#define __EFI_UTIL_H

#include <efi/types.h>

const char *efi_strerror(efi_status_t err);
int efi_errno(efi_status_t err);

int __efivarfs_parse_filename(const char *filename, efi_guid_t *vendor,
			      s16 *name, size_t *namelen);
int efivarfs_parse_filename(const char *filename, efi_guid_t *vendor, s16 **name);

static inline int
efi_guidcmp (efi_guid_t left, efi_guid_t right)
{
	return memcmp(&left, &right, sizeof (efi_guid_t));
}

#endif

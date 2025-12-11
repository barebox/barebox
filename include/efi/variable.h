/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_VARIABLE_H
#define __EFI_VARIABLE_H

#include <efi/types.h>

/*
 * Variable Attributes
 */
#define EFI_VARIABLE_NON_VOLATILE       0x0000000000000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x0000000000000002
#define EFI_VARIABLE_RUNTIME_ACCESS     0x0000000000000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x0000000000000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x0000000000000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x0000000000000020
#define EFI_VARIABLE_APPEND_WRITE	0x0000000000000040

#define EFI_VARIABLE_MASK 	(EFI_VARIABLE_NON_VOLATILE | \
				EFI_VARIABLE_BOOTSERVICE_ACCESS | \
				EFI_VARIABLE_RUNTIME_ACCESS | \
				EFI_VARIABLE_HARDWARE_ERROR_RECORD | \
				EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS | \
				EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | \
				EFI_VARIABLE_APPEND_WRITE)
/*
 * Length of a GUID string (strlen("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"))
 * not including trailing NUL
 */
#define EFI_VARIABLE_GUID_LEN 36

int __efivarfs_parse_filename(const char *filename, efi_guid_t *vendor,
			      s16 *name, size_t *namelen);
int efivarfs_parse_filename(const char *filename, efi_guid_t *vendor, s16 **name);

void *efi_get_variable(char *name, efi_guid_t *vendor, int *var_size);

int efi_set_variable(char *name, efi_guid_t *vendor, uint32_t attributes,
		     void *buf, size_t size);

int efi_set_variable_usec(char *name, efi_guid_t *vendor, uint64_t usec);
int efi_set_variable_printf(char *name, efi_guid_t *vendor, const char *fmt, ...);
int efi_set_variable_uint64_le(char *name, efi_guid_t *vendor, uint64_t value);

static inline void *efi_get_global_var(char *name, int *var_size)
{
	extern efi_guid_t efi_global_variable_guid;
	return efi_get_variable(name, &efi_global_variable_guid, var_size);
}

#endif

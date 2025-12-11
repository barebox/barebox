/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_VARIABLE_H
#define __EFI_VARIABLE_H

#include <efi/types.h>

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

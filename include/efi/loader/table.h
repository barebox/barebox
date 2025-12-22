/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __EFI_LOADER_TABLE_H
#define __EFI_LOADER_TABLE_H

#include <efi/types.h>

/* Adds new or overrides configuration table entry to the system table */
efi_status_t efi_install_configuration_table(const efi_guid_t *guid, void *table);

#endif

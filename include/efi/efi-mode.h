/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_MODE_H
#define __EFI_MODE_H

#include <linux/stddef.h>
#include <linux/types.h>

struct efi_boot_services;
extern struct efi_boot_services *BS;

static inline bool efi_is_payload(void)
{
	return IS_ENABLED(CONFIG_EFI_PAYLOAD) && BS;
}

static inline bool efi_is_loader(void)
{
	return false;
}

static inline struct efi_boot_services *efi_get_boot_services(void)
{
	if (efi_is_payload())
		return BS;

	return NULL;
}

#endif

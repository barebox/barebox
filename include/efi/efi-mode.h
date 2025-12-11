/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_MODE_H
#define __EFI_MODE_H

#include <linux/stddef.h>
#include <linux/types.h>

struct efi_boot_services;
struct efi_runtime_services;
extern struct efi_boot_services *BS;
extern struct efi_runtime_services *RT;

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

static inline struct efi_runtime_services *efi_get_runtime_services(void)
{
	if (efi_is_payload())
		return RT;

	return NULL;
}

#endif

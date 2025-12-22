/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_MODE_H
#define __EFI_MODE_H

#include <linux/stddef.h>
#include <linux/types.h>
#include <efi/types.h>

struct efi_boot_services;
struct efi_runtime_services;
extern struct efi_boot_services *BS, *loaderBS;
extern struct efi_runtime_services *RT, *loaderRT;

enum efi_loader_state {
	EFI_LOADER_INACTIVE = 0,
	EFI_LOADER_BOOT,
	EFI_LOADER_RUNTIME,
};

static inline bool efi_is_payload(void)
{
	return IS_ENABLED(CONFIG_EFI_PAYLOAD) && BS;
}

#ifdef CONFIG_EFI_LOADER
enum efi_loader_state efi_is_loader(void);
#else
static inline enum efi_loader_state efi_is_loader(void)
{
	return EFI_LOADER_INACTIVE;
}
#endif


static inline struct efi_boot_services *efi_get_boot_services(void)
{
	if (efi_is_payload())
		return BS;
	if (efi_is_loader())
		return loaderBS;

	return NULL;
}

static inline struct efi_runtime_services *efi_get_runtime_services(void)
{
	if (efi_is_payload())
		return RT;
	if (efi_is_loader())
		return loaderRT;

	return NULL;
}

#endif

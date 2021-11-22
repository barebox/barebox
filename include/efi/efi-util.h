/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_UTIL_H
#define __EFI_UTIL_H

#include <efi.h>

const char *efi_strerror(efi_status_t err);
int efi_errno(efi_status_t err);

#endif

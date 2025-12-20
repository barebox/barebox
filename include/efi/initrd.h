/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PAYLOAD_INITRD_H
#define __EFI_PAYLOAD_INITRD_H

#include <linux/types.h>

int efi_initrd_register(void *initrd, size_t initrd_size);
void efi_initrd_unregister(void);

#endif

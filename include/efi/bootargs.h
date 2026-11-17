/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_BOOTARGS_H
#define __EFI_BOOTARGS_H

#include <linux/types.h>

char *efi_bootargs_get(bool linux_image);

#endif /* __EFI_BOOTARGS_H */

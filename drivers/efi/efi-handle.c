// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <efi.h>
#include <efi/efi-util.h>
#include <efi/efi-device.h>
#include <efi/efi-mode.h>

int __efi_locate_handle(struct efi_boot_services *bs,
			enum efi_locate_search_type search_type,
			efi_guid_t *protocol,
			void *search_key,
			unsigned long *no_handles,
			efi_handle_t **buffer)
{
	efi_status_t efiret;
	unsigned long buffer_size = 0;
	efi_handle_t *buf;

	efiret = bs->locate_handle(search_type, protocol, search_key, &buffer_size,
			NULL);
	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL)
		return -efi_errno(efiret);

	buf = malloc(buffer_size);
	if (!buf)
		return -ENOMEM;

	efiret = bs->locate_handle(search_type, protocol, search_key, &buffer_size,
			buf);
	if (EFI_ERROR(efiret)) {
		free(buf);
		return -efi_errno(efiret);
	}

	*no_handles = buffer_size / sizeof(efi_handle_t);
	*buffer = buf;

	return 0;
}

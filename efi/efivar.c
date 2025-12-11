// SPDX-License-Identifier: GPL-2.0-only
/*
 * init.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#define pr_fmt(fmt) "efi-var: " fmt

#include <efi/efi-mode.h>
#include <efi/variable.h>
#include <efi.h>
#include <efi/efi-util.h>
#include <efi/variable.h>
#include <wchar.h>
#include <xfuncs.h>
#include <stdio.h>

void *efi_get_variable(char *name, efi_guid_t *vendor, int *var_size)
{
	struct efi_runtime_services *rt = efi_get_runtime_services();
	efi_status_t efiret;
	void *buf;
	size_t size = 0;
	s16 *name16;

	if (!rt)
		return ERR_PTR(-ENODEV);

	name16 = xstrdup_char_to_wchar(name);

	efiret = rt->get_variable(name16, vendor, NULL, &size, NULL);

	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL) {
		buf = ERR_PTR(-efi_errno(efiret));
		goto out;
	}

	buf = malloc(size);
	if (!buf) {
		buf = ERR_PTR(-ENOMEM);
		goto out;
	}

	efiret = rt->get_variable(name16, vendor, NULL, &size, buf);
	if (EFI_ERROR(efiret)) {
		free(buf);
		buf = ERR_PTR(-efi_errno(efiret));
		goto out;
	}

	if (var_size)
		*var_size = size;

out:
	free(name16);

	return buf;
}

int efi_set_variable(char *name, efi_guid_t *vendor, uint32_t attributes,
		     void *buf, size_t size)
{
	struct efi_runtime_services *rt = efi_get_runtime_services();
	efi_status_t efiret = EFI_SUCCESS;
	s16 *name16;

	if (!rt)
		return -ENODEV;

	name16 = xstrdup_char_to_wchar(name);

	efiret = rt->set_variable(name16, vendor, attributes, size, buf);

	free(name16);

	return -efi_errno(efiret);
}

int efi_set_variable_usec(char *name, efi_guid_t *vendor, uint64_t usec)
{
	char buf[20];
	wchar_t buf16[40];

	snprintf(buf, sizeof(buf), "%lld", usec);
	strcpy_char_to_wchar(buf16, buf);

	return efi_set_variable(name, vendor,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS, buf16,
				(strlen(buf)+1) * sizeof(wchar_t));
}

int efi_set_variable_printf(char *name, efi_guid_t *vendor, const char *fmt, ...)
{
	va_list args;
	char *buf;
	wchar_t *buf16;

	va_start(args, fmt);
	buf = xvasprintf(fmt, args);
	va_end(args);
	buf16 = xstrdup_char_to_wchar(buf);

	return efi_set_variable(name, vendor,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS, buf16,
				(strlen(buf)+1) * sizeof(wchar_t));
	free(buf);
	free(buf16);
}

int efi_set_variable_uint64_le(char *name, efi_guid_t *vendor, uint64_t value)
{
	uint8_t buf[8];

	buf[0] = (uint8_t)(value >> 0U & 0xFF);
	buf[1] = (uint8_t)(value >> 8U & 0xFF);
	buf[2] = (uint8_t)(value >> 16U & 0xFF);
	buf[3] = (uint8_t)(value >> 24U & 0xFF);
	buf[4] = (uint8_t)(value >> 32U & 0xFF);
	buf[5] = (uint8_t)(value >> 40U & 0xFF);
	buf[6] = (uint8_t)(value >> 48U & 0xFF);
	buf[7] = (uint8_t)(value >> 56U & 0xFF);

	return efi_set_variable(name, vendor,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS, buf,
				sizeof(buf));
}

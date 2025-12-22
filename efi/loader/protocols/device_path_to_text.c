// SPDX-License-Identifier: GPL-2.0+
/*
 *  EFI device path interface
 *
 *  Copyright (c) 2017 Heinrich Schuchardt
 */

#include <charset.h>
#include <efi/devicepath.h>
#include <efi/loader/devicepath.h>
#include <efi/protocol/devicepath.h>
#include <efi/loader/object.h>
#include <efi/loader/trace.h>
#include <efi/loader.h>
#include <efi/guid.h>
#include <efi/error.h>
#include <init.h>

/**
 * efi_str_to_u16() - convert ASCII string to UTF-16
 *
 * A u16 buffer is allocated from pool. The ASCII string is copied to the u16
 * buffer.
 *
 * @str:	ASCII string
 * Return:	UTF-16 string. NULL if out of memory.
 */
static u16 *efi_str_to_u16(const char *str)
{
	size_t len;
	u16 *out, *dst;
	efi_status_t ret;

	if (!str)
		return NULL;

	len = sizeof(u16) * (utf8_utf16_strlen(str) + 1);
	ret = efi_allocate_pool(EFI_BOOT_SERVICES_DATA, len, (void **)&out, "DP");
	if (ret != EFI_SUCCESS)
		return NULL;
	dst = out;
	utf8_utf16_strcpy(&dst, str);
	return out;
}

static uint16_t EFIAPI *__efi_convert_device_path_to_text(
		const struct efi_device_path *dp,
		bool all_nodes)
{
	char *str;
	uint16_t *text;

	if (!dp)
		return NULL;

	str = device_path_to_str(dp, all_nodes);
	text = efi_str_to_u16(str);
	free(str);

	return text;
}

/*
 * This function implements the ConvertDeviceNodeToText service of the
 * EFI_DEVICE_PATH_TO_TEXT_PROTOCOL.
 * See the Unified Extensible Firmware Interface (UEFI) specification
 * for details.
 *
 * device_node		device node to be converted
 * display_only		true if the shorter text representation shall be used
 * allow_shortcuts	true if shortcut forms may be used
 * Return:		text representation of the device path
 *			NULL if out of memory of device_path is NULL
 */
static uint16_t EFIAPI *efi_convert_device_node_to_text(
		const struct efi_device_path *device_node,
		bool display_only,
		bool allow_shortcuts)
{
	uint16_t *text;

	EFI_ENTRY("%p, %d, %d", device_node, display_only, allow_shortcuts);

	text = __efi_convert_device_path_to_text(device_node, false);

	EFI_EXIT(EFI_SUCCESS);

	return text;
}

/*
 * This function implements the ConvertDevicePathToText service of the
 * EFI_DEVICE_PATH_TO_TEXT_PROTOCOL.
 * See the Unified Extensible Firmware Interface (UEFI) specification
 * for details.
 *
 * device_path		device path to be converted
 * display_only		true if the shorter text representation shall be used
 * allow_shortcuts	true if shortcut forms may be used
 * Return:		text representation of the device path
 *			NULL if out of memory of device_path is NULL
 */
static uint16_t EFIAPI *efi_convert_device_path_to_text(
		const struct efi_device_path *device_path,
		bool display_only,
		bool allow_shortcuts)
{
	uint16_t *text;

	EFI_ENTRY("%p, %d, %d", device_path, display_only, allow_shortcuts);

	text = __efi_convert_device_path_to_text(device_path, true);

	EFI_EXIT(EFI_SUCCESS);

	return text;
}

static const struct efi_device_path_to_text_protocol efi_device_path_to_text = {
	.convert_device_node_to_text = efi_convert_device_node_to_text,
	.convert_device_path_to_text = efi_convert_device_path_to_text,
};

static int efi_device_path_to_text_init(void)
{
	efi_add_root_node_protocol_deferred(&efi_device_path_to_text_protocol_guid,
					    &efi_device_path_to_text);
	return 0;
}
postcore_initcall(efi_device_path_to_text_init);

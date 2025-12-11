// SPDX-License-Identifier: GPL-2.0-only
/*
 * efi-initrd.c - barebox EFI payload support
 *
 * Copyright (c) 2025 Anis Chali <chalianis1@gmail.com>
 * Copyright (C) 2025 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/hw_random.h>
#include <efi/devicepath.h>
#include <efi/protocol/file.h>
#include <efi/initrd.h>
#include <efi/guid.h>
#include <efi/payload.h>
#include <efi/error.h>

static efi_status_t EFIAPI efi_initrd_load_file2(
	struct efi_load_file_protocol *this, struct efi_device_path *file_path,
	bool boot_policy, unsigned long *buffer_size, void *buffer);

static const struct {
	struct efi_device_path_vendor vendor;
	struct efi_device_path end;
} __packed initrd_dev_path = {
	{ {
		  DEVICE_PATH_TYPE_MEDIA_DEVICE,
		  DEVICE_PATH_SUB_TYPE_VENDOR_PATH,
		  sizeof(initrd_dev_path.vendor),
	  },
	EFI_LINUX_INITRD_MEDIA_GUID },
	{ DEVICE_PATH_TYPE_END, DEVICE_PATH_SUB_TYPE_END,
		DEVICE_PATH_END_LENGTH }
};

static struct linux_initrd {
	struct efi_load_file_protocol base;
	void *start;
	size_t size;
	efi_handle_t lf2_handle;
} initrd;

#define to_linux_initrd(x) container_of(x, struct linux_initrd, base)

static efi_status_t EFIAPI efi_initrd_load_file2(
	struct efi_load_file_protocol *this, struct efi_device_path *file_path,
	bool boot_policy, unsigned long *buffer_size, void *buffer)
{

	struct linux_initrd *initrd = to_linux_initrd(this);

	if (!this || this != &initrd->base || !buffer_size)
		return EFI_INVALID_PARAMETER;

	if (file_path->type != initrd_dev_path.end.type ||
	    file_path->sub_type != initrd_dev_path.end.sub_type)
		return EFI_INVALID_PARAMETER;

	if (boot_policy)
		return EFI_UNSUPPORTED;

	if (!buffer || *buffer_size < initrd->size) {
		*buffer_size = initrd->size;
		return EFI_BUFFER_TOO_SMALL;
	} else {
		memcpy(buffer, initrd->start, initrd->size);
		*buffer_size = initrd->size;
	}

	return EFI_SUCCESS;
}

int efi_initrd_register(void *initrd_base, size_t initrd_sz)
{
	efi_status_t efiret;
	int ret;

	initrd.base.load_file = efi_initrd_load_file2;
	initrd.start = initrd_base;
	initrd.size = initrd_sz;

	efiret = BS->install_multiple_protocol_interfaces(
		&initrd.lf2_handle, &efi_load_file2_protocol_guid, &initrd.base,
		&efi_device_path_protocol_guid, &initrd_dev_path, NULL);
	if (EFI_ERROR(efiret)) {
		pr_err("Failed to install protocols for INITRD %s\n",
		       efi_strerror(efiret));
		ret = -efi_errno(efiret);
		return ret;
	}

	return 0;
}

void efi_initrd_unregister(void)
{
	if (!initrd.base.load_file)
		return;

	BS->uninstall_multiple_protocol_interfaces(
		initrd.lf2_handle, &efi_device_path_protocol_guid, &initrd_dev_path,
		&efi_load_file2_protocol_guid, &initrd.base, NULL);

	initrd.base.load_file = NULL;
}

// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/bc4fe5666dae6ab01a433970c3f5e6eb4833ebe7/lib/efi_loader/efi_var_file.c
/*
 * File interface for UEFI variables
 *
 * Copyright (c) 2020, Heinrich Schuchardt
 */

#define pr_fmt(fmt) "efi-loader: var-file: " fmt

#include <linux/kernel.h>
#include <efi/loader.h>
#include <efi/mode.h>
#include <efi/loader/variable.h>
#include <efi/variable.h>
#include <efi/runtime.h>
#include <efi/error.h>
#include <efi/guid.h>
#include <stdlib.h>
#include <crc.h>
#include <fs.h>
#include <libfile.h>
#include <string.h>
#include <globalvar.h>
#include <magicvar.h>

#include "variable.h"

/* Filename within ESP to save/restore EFI variables from
 * when CONFIG_EFI_VARIABLE_FILE_STORE=y
 */
char *efi_var_file_name;

/**
 * efi_var_to_file() - save non-volatile variables as file
 *
 * File indiciated by efi_var_file_name with EFI variables is
 * created on the EFI system partion.
 *
 * Return:	status code
 */
efi_status_t efi_var_to_file(void)
{
	efi_status_t efiret;
	struct efi_var_file *buf;
	loff_t len;
	int err, fd, dirfd;
	static bool once;

	if (!IS_ENABLED(CONFIG_EFI_VARIABLE_FILE_STORE))
		return EFI_SUCCESS;

	efiret = efi_var_collect(&buf, &len, EFI_VARIABLE_NON_VOLATILE);
	if (efiret != EFI_SUCCESS) {
		err = ENOMEM;
		goto error;
	}

	dirfd = efiloader_esp_mount_dir();
	if (dirfd < 0) {
		if (!once) {
			pr_warn("Cannot persist EFI variables without system partition\n");
			once = true;
		}
		efiret = EFI_NO_MEDIA;
		err = dirfd;
		goto out;
	}

	// TODO: replace with rename
	fd = openat(dirfd, efi_var_file_name, O_WRONLY | O_TRUNC | O_CREAT);
	if (fd < 0) {
		efiret = EFI_DEVICE_ERROR;
		err = fd;
		goto error;
	}

	err = write_full(fd, buf, len);

	close(fd);

	if (err < 0) {
		efiret = EFI_DEVICE_ERROR;
		goto error;
	}

	return 0;

error:
	if (efiret != EFI_SUCCESS)
		pr_err("Failed to persist EFI variables %pe\n", ERR_PTR(err));

out:
	free(buf);
	return efiret;
}

static efi_status_t efi_var_restore(struct efi_var_file *buf, bool safe)
{
	struct efi_var_entry *var, *last_var;
	u16 *data;
	efi_status_t ret;

	if (buf->reserved || buf->magic != EFI_VAR_FILE_MAGIC ||
	    buf->crc32 != crc32(0, (u8 *)buf->var,
				buf->length - sizeof(struct efi_var_file))) {
		pr_err("Invalid EFI variables file\n");
		return EFI_INVALID_PARAMETER;
	}

	last_var = (struct efi_var_entry *)((u8 *)buf + buf->length);
	for (var = buf->var; var < last_var;
	     var = (struct efi_var_entry *)
		   ALIGN((uintptr_t)data + var->length, 8)) {

		data = var->name + wcslen(var->name) + 1;

		/*
		 * Secure boot related and volatile variables shall only be
		 * restored from the preseed, but we didn't implement this yet.
		 */
		if (!safe &&
		    (efi_auth_var_get_type(var->name, &var->guid) !=
		     EFI_AUTH_VAR_NONE ||
		     !efi_guidcmp(var->guid, shim_lock_guid) ||
		     !(var->attr & EFI_VARIABLE_NON_VOLATILE)))
			continue;
		if (!var->length)
			continue;
		if (efi_var_mem_find(&var->guid, var->name, NULL))
			continue;
		ret = efi_var_mem_ins(var->name, &var->guid, var->attr,
				      var->length, data, 0, NULL,
				      var->time);
		if (ret != EFI_SUCCESS)
			pr_err("Failed to set EFI variable %ls\n", var->name);
	}
	return EFI_SUCCESS;
}

/**
 * efi_var_from_file() - read variables from file
 *
 * Specified file is read from the EFI system partitions and the variables
 * stored in the file are created.
 *
 * On first boot the file may not exist yet. This is why we must
 * return EFI_SUCCESS in this case.
 *
 * If the variable file is corrupted, e.g. incorrect CRC32, we do not want to
 * stop the boot process. We deliberately return EFI_SUCCESS in this case, too.
 *
 * @dirfd	directory fd
 * @filename	name of the file to load variables from
 * Return:	status code
 */
efi_status_t efi_var_from_file(int dirfd, const char *filename)
{
	struct efi_var_file *buf;
	efi_status_t ret;
	size_t len;
	int fd;

	fd = openat(dirfd, filename, O_RDONLY);
	if (fd < 0)
		return EFI_NOT_FOUND;

	buf = read_fd(fd, &len);

	close(fd);

	if (!buf || len < sizeof(struct efi_var_file)) {
		ret = EFI_NOT_FOUND;
		goto error;
	}

	if (buf->length != len || efi_var_restore(buf, false) != EFI_SUCCESS) {
		ret = EFI_CRC_ERROR;
		goto error;
	}

	return EFI_SUCCESS;
error:
	free(buf);
	return ret;
}

// SPDX-SnippetBegin
// SPDX-Snippet-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/e9c34fab18a9a0022b36729afd8e262e062764e2/lib/efi_loader/efi_runtime.c

efi_status_t efi_init_runtime_variable_supported(void)
{
	u8 s = 0;
	int ret;

	if (!IS_ENABLED(CONFIG_EFI_RT_VOLATILE_STORE))
		return EFI_SUCCESS;

	ret = efi_set_variable_int(u"RTStorageVolatile",
				   &efi_file_store_vars_guid,
				   EFI_VARIABLE_BOOTSERVICE_ACCESS |
				   EFI_VARIABLE_RUNTIME_ACCESS |
				   EFI_VARIABLE_READ_ONLY,
				   strlen(efi_var_file_name) + 1,
				   efi_var_file_name, false);
	if (ret != EFI_SUCCESS) {
		pr_err("Failed to set RTStorageVolatile\n");
		return ret;
	}
	/*
	 * This variable needs to be visible so users can read it,
	 * but the real contents are going to be filled during
	 * GetVariable
	 */
	ret = efi_set_variable_int(u"VarToFile",
				   &efi_file_store_vars_guid,
				   EFI_VARIABLE_BOOTSERVICE_ACCESS |
				   EFI_VARIABLE_RUNTIME_ACCESS |
				   EFI_VARIABLE_READ_ONLY,
				   sizeof(s),
				   &s, false);
	if (ret != EFI_SUCCESS) {
		pr_err("Failed to set VarToFile\n");
		efi_set_variable_int(u"RTStorageVolatile",
				     &efi_file_store_vars_guid,
				     EFI_VARIABLE_BOOTSERVICE_ACCESS |
				     EFI_VARIABLE_RUNTIME_ACCESS |
				     EFI_VARIABLE_READ_ONLY,
				     0, NULL, false);
		return ret;
	}

	return EFI_SUCCESS;
}

// SPDX-SnippetEnd

static int efi_init_var_params(void)
{
	if (efi_is_payload())
		return 0;

	/* Not 8.3, but brboxefi.var looks just too ugly */
	efi_var_file_name = xstrdup("barebox.efivars");
	dev_add_param_string(&efidev, "vars.filestore", NULL, NULL,
			     &efi_var_file_name, NULL);


	return 0;
}
late_initcall(efi_init_var_params);

BAREBOX_MAGICVAR(efi.vars.filestore,
		 "efiloader: Name of file within ESP to store variables to (WARNING: Not power-fail safe!)");

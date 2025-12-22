// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/530e869ff89d9575103637af201fe97864d4f577/lib/efi_loader/efi_var_mem.c
/*
 * File interface for UEFI variables
 *
 * Copyright (c) 2020, Heinrich Schuchardt
 */

#define pr_fmt(fmt) "efi-loader: variable-mem: " fmt

#include <linux/kernel.h>
#include <linux/sizes.h>
#include <efi/loader/variable.h>
#include <efi/runtime.h>
#include <efi/mode.h>
#include <efi/services.h>
#include <efi/loader.h>
#include <efi/variable.h>
#include <efi/error.h>
#include <param.h>
#include <magicvar.h>

static unsigned efi_var_buf_size = SZ_128K;

efi_status_t efi_var_mem_init(void)
{
	struct efi_var_file *efi_var_buf;
	u64 memory;
	efi_status_t ret;

	ret = efi_allocate_pages(EFI_ALLOCATE_ANY_PAGES,
				 EFI_RUNTIME_SERVICES_DATA,
				 efi_size_in_pages(efi_var_buf_size),
				 &memory, "vars");
	if (ret != EFI_SUCCESS)
		return ret;
	efi_var_buf = (struct efi_var_file *)(uintptr_t)memory;
	memset(efi_var_buf, 0,  efi_var_buf_size);
	efi_var_buf->magic = EFI_VAR_FILE_MAGIC;
	efi_var_buf->length = (uintptr_t)efi_var_buf->var -
			      (uintptr_t)efi_var_buf;

	/* we would have to register EFI_EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE otherwise */
	static_assert(!IS_ENABLED(EFI_RUNTIME_SET_VIRTUAL_ADDRESS_MAP));

	efi_var_buf_set(efi_var_buf, efi_var_buf_size);

	return EFI_SUCCESS;
}

void efi_var_buf_update(struct efi_var_file *var_buf)
{
	unsigned len;
	struct efi_var_file *dst = efi_var_buf_get(&len);

	memcpy(dst, var_buf, len);
}

static int efi_var_buf_size_set(struct param_d *param, void *priv)
{
	if (!efi_var_buf_size || efi_var_buf_size % EFI_PAGE_SIZE)
		return -EINVAL;

	return 0;
}

static int efi_init_var_params(void)
{
	if (efi_is_payload())
		return 0;

	dev_add_param_uint32(&efidev, "vars.bufsize", efi_var_buf_size_set, NULL,
			     &efi_var_buf_size, "%u", NULL);

	return 0;
}
late_initcall(efi_init_var_params);

BAREBOX_MAGICVAR(efi.vars.bufsize,
		 "efiloader: Size in bytes of the memory area reserved for keeping UEFI variables");

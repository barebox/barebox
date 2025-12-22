// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: 2016 Alexander Graf
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/1c5aab803c0b0f07be1d3b0029f4031463497acf/lib/efi_loader/efi_smbios.c

#define pr_fmt(fmt) "efi-loader: smbios: " fmt

#include <efi/loader.h>
#include <efi/guid.h>
#include <efi/error.h>
#include <efi/memory.h>
#include <efi/loader/table.h>
#include <linux/printk.h>
#include <malloc.h>
#include <smbios.h>
#include <init.h>
#include <linux/sizes.h>

#define TABLE_SIZE	SZ_4K

/*
 * Install the SMBIOS table as a configuration table.
 *
 * Return:	status code
 */
static efi_status_t efi_smbios_register(void *data)
{
	efi_status_t ret;
	u64 memory;
	void *buf;

	/* Align the table to a 4KB boundary to keep EFI happy */
	ret = efi_allocate_pages(EFI_ALLOCATE_ANY_PAGES,
				 EFI_RUNTIME_SERVICES_DATA,
				 efi_size_in_pages(TABLE_SIZE),
				 &memory, "smbios");
	if (ret != EFI_SUCCESS)
		return ret;

	buf = efi_phys_to_virt(memory);

	if (!write_smbios_table(buf)) {
		pr_err("Failed to write SMBIOS table\n");
		return -EINVAL;
	}

	/* Install SMBIOS information as configuration table */
	return efi_install_configuration_table(&efi_smbios3_guid, buf);
}

static int efi_smbios_init(void)
{
	efi_register_deferred_init(efi_smbios_register, NULL);
	return 0;
}
postenvironment_initcall(efi_smbios_init);

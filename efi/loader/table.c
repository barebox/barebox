// SPDX-License-Identifier: GPL-2.0

#include <efi/services.h>
#include <efi/loader.h>
#include <efi/error.h>
#include <efi/attributes.h>
#include <crc.h>

/**
 * efi_update_table_header_crc32() - Update crc32 in table header
 *
 * @table:	EFI table
 */
void __efi_runtime efi_update_table_header_crc32(struct efi_table_hdr *table)
{
	table->crc32 = __pi_crc32(0, table, table->headersize);
}

/**
 * efi_unimplemented() - replacement function, returns EFI_UNSUPPORTED
 *
 * This function is used after SetVirtualAddressMap() is called as replacement
 * for services that are not available anymore due to constraints of our
 * implementation.
 *
 * Return:	EFI_UNSUPPORTED
 */
efi_status_t __efi_runtime EFIAPI efi_unimplemented(void)
{
	return EFI_UNSUPPORTED;
}

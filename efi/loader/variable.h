/* SPDX-License-Identifier: GPL-2.0 */
#include <efi/types.h>
#include <efi/error.h>

efi_status_t efi_init_runtime_variable_supported(void);

efi_status_t EFIAPI efi_get_variable_boot(u16 *variable_name,
					  const efi_guid_t *vendor, u32 *attributes,
					  efi_uintn_t *data_size, void *data);

efi_status_t EFIAPI efi_set_variable_boot(u16 *variable_name,
					  const efi_guid_t *vendor, u32 attributes,
					  efi_uintn_t data_size, const void *data);

efi_status_t EFIAPI efi_get_next_variable_name_boot(efi_uintn_t *variable_name_size,
						    u16 *variable_name,
						    efi_guid_t *vendor);

efi_status_t EFIAPI efi_query_variable_info_boot(
			u32 attributes, u64 *maximum_variable_storage_size,
			u64 *remaining_variable_storage_size,
			u64 *maximum_variable_size);

efi_status_t efi_init_secure_state(void);

bool efi_secure_boot_enabled(void);

void *efi_get_var(const u16 *name, const efi_guid_t *vendor, efi_uintn_t *size);

struct efi_var_file;

efi_status_t efi_var_collect(struct efi_var_file **bufp, loff_t *lenp,
			     u32 check_attr_mask);

efi_status_t efi_var_to_file(void);

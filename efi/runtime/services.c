// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum <a.fatoum@pengutronix.de>
/*
 *  EFI application runtime services - detached callbacks after
 *  boot services have been exited.
 */

/* LSP-Workaround as our compile_commands.json target doesn't handle lib-y yet */
#include "common.h"

#include <efi/loader.h>
#include <efi/runtime.h>
#include <efi/loader/variable.h>
#include <efi/variable.h>
#include <efi/error.h>
#include <generated/version.h>
#include <poweroff.h>
#include <restart.h>
#include <barebox.h>
#include <debug_ll.h>

/**
 * efirt_unimplemented() - replacement macro, returns EFI_UNSUPPORTED
 *
 * This macro is used after SetVirtualAddressMap() to be called as replacement
 * for services that are not available anymore due to constraints of the barebox
 * implementation.
 *
 * Return:	EFI_UNSUPPORTED
 */
#define efirt_unimplemented() ({	\
	EFI_ENTRY(L"");			\
	EFI_EXIT(EFI_UNSUPPORTED);	\
})

/**
 * efirt_reset_system() - reset system at runtime
 *
 * This function implements the ResetSystem() runtime service after
 * SetVirtualAddressMap() is called. As this placeholder cannot reset the
 * system it simply return to the caller.
 *
 * Boards may override the helpers below to implement reset functionality.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @reset_type:		type of reset to perform
 * @reset_status:	status code for the reset
 * @data_size:		size of reset_data
 * @reset_data:		information about the reset
 */
static void EFIAPI efirt_reset_system(enum efi_reset_type reset_type,
				    efi_status_t reset_status,
				    size_t data_size, void *reset_data)
{
	switch (reset_type) {
	case EFI_RESET_WARM:
		EFI_ENTRY(L"warm");
		rt_restart_machine(RESTART_WARM);
		break;
	case EFI_RESET_COLD:
		EFI_ENTRY(L"cold");
		rt_restart_machine(0);
		break;
	case EFI_RESET_PLATFORM_SPECIFIC:
		EFI_ENTRY(L"platform");
		rt_restart_machine(0);
		break;
	case EFI_RESET_SHUTDOWN:
		EFI_ENTRY(L"shutdown");
		rt_poweroff_machine(0);
		break;
	}

	__hang();
}

/**
 * efirt_get_time() - get current time
 *
 * This function implements the GetTime runtime service after
 * SetVirtualAddressMap() is called. As the barebox driver are not available
 * anymore only an error code is returned.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification
 * for details.
 *
 * @time:		pointer to structure to receive current time
 * @capabilities:	pointer to structure to receive RTC properties
 * Returns:		status code
 */
static efi_status_t EFIAPI efirt_get_time(
			struct efi_time *time,
			struct efi_time_cap *capabilities)
{
	return efirt_unimplemented();
}

/**
 * efirt_set_time() - set current time
 *
 * This function implements the SetTime runtime service after
 * SetVirtualAddressMap() is called. As the barebox driver are not available
 * anymore only an error code is returned.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification
 * for details.
 *
 * @time:		pointer to structure to with current time
 * Returns:		status code
 */
static efi_status_t EFIAPI efirt_set_time(struct efi_time *time)
{
	return efirt_unimplemented();
}

/**
 * efirt_update_capsule_unsupported() - process information from operating system
 *
 * This function implements the UpdateCapsule() runtime service.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @capsule_header_array:	pointer to array of virtual pointers
 * @capsule_count:		number of pointers in capsule_header_array
 * @scatter_gather_list:	pointer to array of physical pointers
 * Returns:			status code
 */
static efi_status_t EFIAPI efirt_update_capsule_unsupported(
			struct efi_capsule_header **capsule_header_array,
			size_t capsule_count,
			u64 scatter_gather_list)
{
	return efirt_unimplemented();
}

/**
 * efirt_query_capsule_caps_unsupported() - check if capsule is supported
 *
 * This function implements the QueryCapsuleCapabilities() runtime service.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @capsule_header_array:	pointer to array of virtual pointers
 * @capsule_count:		number of pointers in capsule_header_array
 * @maximum_capsule_size:	maximum capsule size
 * @reset_type:			type of reset needed for capsule update
 * Returns:			status code
 */
static efi_status_t EFIAPI efirt_query_capsule_caps_unsupported(
			struct efi_capsule_header **capsule_header_array,
			size_t capsule_count,
			u64 *maximum_capsule_size,
			u32 *reset_type)
{
	return efirt_unimplemented();
}

static efi_status_t EFIAPI efirt_get_wakeup_time(char *enabled, char *pending,
					       struct efi_time *time)
{
	return efirt_unimplemented();
}

static efi_status_t EFIAPI efirt_set_wakeup_time(char enabled,
						 struct efi_time *time)
{
	return efirt_unimplemented();
}

static efi_status_t EFIAPI efirt_set_virtual_address_map(size_t memory_map_size,
							 size_t descriptor_size,
							 uint32_t descriptor_version,
							 struct efi_memory_desc *virtmap)
{
	return efirt_unimplemented();
}

static efi_status_t EFIAPI efirt_convert_pointer(unsigned long dbg, void **address)
{
	return efirt_unimplemented();
}

static efi_status_t EFIAPI
efirt_get_variable(u16 *variable_name, const efi_guid_t *guid,
		   u32 *attributes, efi_uintn_t *data_size, void *data)
{
	efi_status_t ret;

	EFI_ENTRY(variable_name);

	ret = efi_get_variable_mem(variable_name, guid, attributes, data_size,
				   data, NULL, EFI_VARIABLE_RUNTIME_ACCESS);

	/* Remove EFI_VARIABLE_READ_ONLY flag */
	if (attributes)
		*attributes &= EFI_VARIABLE_MASK;

	return EFI_EXIT(ret);
}

static efi_status_t EFIAPI
efirt_get_next_variable_name(efi_uintn_t *variable_name_size,
			     u16 *variable_name, efi_guid_t *guid)
{
	efi_status_t ret;

	EFI_ENTRY(variable_name);

	ret = efi_get_next_variable_name_mem(variable_name_size, variable_name,
					      guid, EFI_VARIABLE_RUNTIME_ACCESS);

	return EFI_EXIT(ret);
}

/**
 * efirt_query_variable_info_runtime() - runtime implementation of
 *				         QueryVariableInfo()
 *
 * @attributes:				bitmask to select variables to be
 *					queried
 * @maximum_variable_storage_size:	maximum size of storage area for the
 *					selected variable types
 * @remaining_variable_storage_size:	remaining size of storage are for the
 *					selected variable types
 * @maximum_variable_size:		maximum size of a variable of the
 *					selected type
 * Returns:				status code
 */
static efi_status_t EFIAPI
efirt_query_variable_info(u32 attributes,
			  u64 *maximum_variable_storage_size,
			  u64 *remaining_variable_storage_size,
			  u64 *maximum_variable_size)
{
	efi_status_t ret;

	EFI_ENTRY(L"");

	if (!(attributes & EFI_VARIABLE_RUNTIME_ACCESS))
		return EFI_INVALID_PARAMETER;
	if ((attributes & (EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS |
			   EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS |
			   EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS)))
		return EFI_UNSUPPORTED;

	ret = efi_query_variable_info_int(attributes,
					  maximum_variable_storage_size,
					  remaining_variable_storage_size,
					  maximum_variable_size);

	return EFI_EXIT(ret);
}

static efi_status_t EFIAPI efirt_get_next_high_mono_count(uint32_t *high_count)
{
	return efirt_unimplemented();
}

static struct efi_runtime_services efi_runtime_services_detached = {
	.hdr = {
		.signature = EFI_RUNTIME_SERVICES_SIGNATURE,
		.revision = EFI_SPECIFICATION_VERSION,
		.headersize = sizeof(struct efi_runtime_services),
	},
	.get_time = efirt_get_time,
	.set_time = efirt_set_time,
	.get_wakeup_time = efirt_get_wakeup_time,
	.set_wakeup_time = efirt_set_wakeup_time,
	.set_virtual_address_map = efirt_set_virtual_address_map,
	.convert_pointer = efirt_convert_pointer,
	.get_variable = efirt_get_variable,
	.get_next_variable = efirt_get_next_variable_name,
	.set_variable = efirt_set_variable,
	.get_next_high_mono_count = efirt_get_next_high_mono_count,
	.reset_system = efirt_reset_system,
	.update_capsule = efirt_update_capsule_unsupported,
	.query_capsule_caps = efirt_query_capsule_caps_unsupported,
	.query_variable_info = efirt_query_variable_info,
};

void efi_runtime_detach(struct efi_system_table *systab)
{
	/* Disable boot time services */
	systab->con_in_handle = NULL;
	systab->con_in = NULL;
	systab->con_out_handle = NULL;
	systab->con_out = NULL;
	systab->stderr_handle = NULL;
	systab->std_err = NULL;
	systab->boottime = NULL;

	systab->runtime = &efi_runtime_services_detached;

	/* Recalculate CRC32 */
	efi_update_table_header_crc32(&systab->hdr);
	efi_update_table_header_crc32(&systab->runtime->hdr);

	puts_ll("detached\n");
}

// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/58d825fb18053ec7e432de7bbb70a452b9228076/lib/efi_loader/efi_variable.c
/*
 * UEFI runtime variable services
 *
 * Copyright (c) 2017 Rob Clark
 */

#include <linux/kernel.h>
#include <efi/loader/variable.h>
#include <efi/services.h>
#include <efi/variable.h>
#include <efi/loader.h>
#include <efi/error.h>
#include <efi/runtime.h>
#include <charset.h>
#include <stdlib.h>

efi_status_t __efi_runtime
efi_get_variable_int(const u16 *variable_name, const efi_guid_t *vendor,
		     u32 *attributes, efi_uintn_t *data_size, void *data,
		     u64 *timep)
{
	return efi_get_variable_mem(variable_name, vendor, attributes, data_size,
				    data, timep, 0);
}

efi_status_t __efi_runtime
efi_get_next_variable_name_int(efi_uintn_t *variable_name_size,
			       u16 *variable_name, efi_guid_t *vendor)
{
	return efi_get_next_variable_name_mem(variable_name_size, variable_name,
					      vendor, 0);
}

/**
 * efi_setvariable_allowed() - checks defined by the UEFI spec for setvariable
 *
 * @variable_name:	name of the variable
 * @vendor:		vendor GUID
 * @attributes:		attributes of the variable
 * @data_size:		size of the buffer with the variable value
 * @data:		buffer with the variable value
 * Return:		status code
 */
efi_status_t __efi_runtime
efi_setvariable_allowed(const u16 *variable_name, const efi_guid_t *vendor,
		    u32 attributes, efi_uintn_t data_size, const void *data)
{
	if (!variable_name || !*variable_name || !vendor)
		return EFI_INVALID_PARAMETER;

	if (data_size && !data)
		return EFI_INVALID_PARAMETER;

	/*
	 * EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated.
	 * We don't support EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS.
	 */
	if (attributes & (EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS | \
			  EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS))
		return EFI_UNSUPPORTED;

	/* Make sure if runtime bit is set, boot service bit is set also */
	if ((attributes &
	     (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) ==
	    EFI_VARIABLE_RUNTIME_ACCESS)
		return EFI_INVALID_PARAMETER;

	/* only EFI_VARIABLE_NON_VOLATILE attribute is invalid */
	if ((attributes & EFI_VARIABLE_MASK) == EFI_VARIABLE_NON_VOLATILE)
		return EFI_INVALID_PARAMETER;

	/* Make sure HR is set with NV, BS and RT */
	if (attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD &&
	    (!(attributes & EFI_VARIABLE_NON_VOLATILE) ||
	     !(attributes & EFI_VARIABLE_RUNTIME_ACCESS) ||
	     !(attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS)))
		return EFI_INVALID_PARAMETER;

	return EFI_SUCCESS;
}

efi_status_t __efi_runtime
efi_query_variable_info_int(u32 attributes,
			    u64 *maximum_variable_storage_size,
			    u64 *remaining_variable_storage_size,
			    u64 *maximum_variable_size)
{
	unsigned size;

	if (!maximum_variable_storage_size ||
	    !remaining_variable_storage_size ||
	    !maximum_variable_size || !attributes)
		return EFI_INVALID_PARAMETER;

	/* EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated */
	if ((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) ||
	    ((attributes & EFI_VARIABLE_MASK) == 0))
		return EFI_UNSUPPORTED;

	if ((attributes & EFI_VARIABLE_MASK) == EFI_VARIABLE_NON_VOLATILE)
		return EFI_INVALID_PARAMETER;

	/* Make sure if runtime bit is set, boot service bit is set also. */
	if ((attributes &
	     (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) ==
	    EFI_VARIABLE_RUNTIME_ACCESS)
		return EFI_INVALID_PARAMETER;

	if (attributes & ~EFI_VARIABLE_MASK)
		return EFI_INVALID_PARAMETER;

	efi_var_buf_get(&size);

	*maximum_variable_storage_size = size - sizeof(struct efi_var_file);
	*remaining_variable_storage_size = efi_var_mem_free();
	*maximum_variable_size = size - sizeof(struct efi_var_file) -
				 sizeof(struct efi_var_entry);
	return EFI_SUCCESS;
}

/**
 * efirt_set_variable() - runtime implementation of SetVariable()
 *
 * @variable_name:	name of the variable
 * @vendor:		vendor GUID
 * @attributes:		attributes of the variable
 * @data_size:		size of the buffer with the variable value
 * @data:		buffer with the variable value
 * Return:		status code
 */
efi_status_t __efi_runtime EFIAPI
efirt_set_variable(u16 *variable_name, const efi_guid_t *vendor,
		   u32 attributes, efi_uintn_t data_size,
		   const void *data)
{
	struct efi_var_entry *var;
	efi_uintn_t ret;
	bool append, delete;
	u64 time = 0;

	if (!IS_ENABLED(CONFIG_EFI_RT_VOLATILE_STORE))
		return EFI_UNSUPPORTED;

	/*
	 * Authenticated variables are not supported. The EFI spec
	 * in §32.3.6 requires keys to be stored in non-volatile storage which
	 * is tamper and delete resistant.
	 * The rest of the checks are in efi_setvariable_allowed()
	 */
	if (attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
		return EFI_INVALID_PARAMETER;

	ret = efi_setvariable_allowed(variable_name, vendor, attributes, data_size,
				      data);
	if (ret != EFI_SUCCESS)
		return ret;

	/* check if a variable exists */
	var = efi_var_mem_find(vendor, variable_name, NULL);
	append = !!(attributes & EFI_VARIABLE_APPEND_WRITE);
	attributes &= ~EFI_VARIABLE_APPEND_WRITE;
	delete = !append && (!data_size || !attributes);

	/* BS only variables are hidden deny writing them */
	if (!delete && !(attributes & EFI_VARIABLE_RUNTIME_ACCESS))
		return EFI_INVALID_PARAMETER;

	if (var) {
		if (var->attr & EFI_VARIABLE_READ_ONLY ||
		    !(var->attr & EFI_VARIABLE_NON_VOLATILE))
			return EFI_WRITE_PROTECTED;

		/* attributes won't be changed */
		if (!delete && (((var->attr & ~EFI_VARIABLE_READ_ONLY) !=
		    (attributes & ~EFI_VARIABLE_READ_ONLY))))
			return EFI_INVALID_PARAMETER;
		time = var->time;
	} else {
		if (!(attributes & EFI_VARIABLE_NON_VOLATILE))
			return EFI_INVALID_PARAMETER;
		if (append && !data_size)
			return EFI_SUCCESS;
		if (delete)
			return EFI_NOT_FOUND;
	}

	if (delete) {
		/* EFI_NOT_FOUND has been handled before */
		attributes = var->attr;
		ret = EFI_SUCCESS;
	} else if (append && var) {
		u16 *old_data = (void *)((uintptr_t)var->name +
			sizeof(u16) * (u16_strlen(var->name) + 1));

		ret = efi_var_mem_ins(variable_name, vendor, attributes,
				      var->length, old_data, data_size, data,
				      time);
	} else {
		ret = efi_var_mem_ins(variable_name, vendor, attributes,
				      data_size, data, 0, NULL, time);
	}

	if (ret != EFI_SUCCESS)
		return ret;
	/* We are always inserting new variables, get rid of the old copy */
	efi_var_mem_del(var);

	return EFI_SUCCESS;
}

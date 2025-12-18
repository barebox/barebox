// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/58d825fb18053ec7e432de7bbb70a452b9228076/lib/efi_loader/efi_variable.c
/*
 * UEFI runtime variable services
 *
 * Copyright (c) 2017 Rob Clark
 */

#define pr_fmt(fmt) "efi-loader: variable: " fmt

#include <linux/kernel.h>
#include <efi/loader.h>
#include <efi/error.h>
#include <efi/variable.h>
#include <efi/runtime.h>
#include <efi/loader/trace.h>
#include <efi/loader/variable.h>
#include <stdlib.h>
#include <crc.h>
#include <fcntl.h>

#include "variable.h"

/* Initial set of efi variables. This is not written back and
 * usually from barebox' built-in env
 */
#define BAREBOX_ENV_INIT_EFIVARS	"/env/data/init.efivars"

/**
 * efi_variable_authenticate - authenticate a variable
 * @variable:	Variable name in u16
 * @vendor:	Guid of variable
 * @data_size:	Size of @data
 * @data:	Pointer to variable's value
 * @given_attr:	Attributes to be given at SetVariable()
 * @env_attr:	Attributes that an existing variable holds
 * @time:	signed time that an existing variable holds
 *
 *
 * Return:	success as variable auth is not yet supported
 */
static efi_status_t efi_variable_authenticate(const u16 *variable,
					      const efi_guid_t *vendor,
					      efi_uintn_t *data_size,
					      const void **data, u32 given_attr,
					      u32 *env_attr, u64 *time)
{
	return EFI_SUCCESS;
}

efi_status_t efi_set_variable_int(const u16 *variable_name,
				  const efi_guid_t *vendor,
				  u32 attributes, efi_uintn_t data_size,
				  const void *data, bool ro_check)
{
	struct efi_var_entry *var;
	efi_uintn_t ret;
	bool append, delete;
	u64 time = 0;
	enum efi_auth_var_type var_type;

	ret = efi_setvariable_allowed(variable_name, vendor, attributes, data_size,
				  data);
	if (ret != EFI_SUCCESS)
		return ret;

	/* check if a variable exists */
	var = efi_var_mem_find(vendor, variable_name, NULL);
	append = !!(attributes & EFI_VARIABLE_APPEND_WRITE);
	delete = !append && (!data_size || !attributes);

	/* check attributes */
	var_type = efi_auth_var_get_type(variable_name, vendor);
	if (var) {
		if (ro_check && (var->attr & EFI_VARIABLE_READ_ONLY))
			return EFI_WRITE_PROTECTED;

		if (var_type >= EFI_AUTH_VAR_PK)
			return EFI_WRITE_PROTECTED;

		/* attributes won't be changed */
		if (!delete &&
		    ((ro_check && var->attr != (attributes & ~EFI_VARIABLE_APPEND_WRITE)) ||
		     (!ro_check && ((var->attr & ~EFI_VARIABLE_READ_ONLY)
				    != (attributes & ~EFI_VARIABLE_READ_ONLY))))) {
			return EFI_INVALID_PARAMETER;
		}
		time = var->time;
	} else {
		/*
		 * UEFI specification does not clearly describe the expected
		 * behavior of append write with data size 0, we follow
		 * the EDK II reference implementation.
		 */
		if (append && !data_size)
			return EFI_SUCCESS;

		/*
		 * EFI_VARIABLE_APPEND_WRITE to non-existent variable is accepted
		 * and new variable is created in EDK II reference implementation.
		 * We follow it and only check the deletion here.
		 */
		if (delete)
			/* Trying to delete a non-existent variable. */
			return EFI_NOT_FOUND;
	}

	if (var_type >= EFI_AUTH_VAR_PK) {
		/* authentication is mandatory */
		if (!(attributes &
		      EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)) {
			EFI_PRINT("%ls: TIME_BASED_AUTHENTICATED_WRITE_ACCESS required\n",
				  variable_name);
			return EFI_INVALID_PARAMETER;
		}
	}

	/* authenticate a variable */
	if (IS_ENABLED(CONFIG_EFI_LOADER_SECURE_BOOT)) {
		if (attributes &
		    EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
			u32 env_attr;

			ret = efi_variable_authenticate(variable_name, vendor,
							&data_size, &data,
							attributes, &env_attr,
							&time);
			if (ret != EFI_SUCCESS)
				return ret;

			/* last chance to check for delete */
			if (!data_size)
				delete = true;
		}
	} else {
		if (attributes &
		    EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
			EFI_PRINT("Secure boot is not configured\n");
			return EFI_INVALID_PARAMETER;
		}
	}

	if (delete) {
		/* EFI_NOT_FOUND has been handled before */
		attributes = var->attr;
		ret = EFI_SUCCESS;
	} else if (append && var) {
		/*
		 * data is appended if EFI_VARIABLE_APPEND_WRITE is set and
		 * variable exists.
		 */
		u16 *old_data = var->name;

		for (; *old_data; ++old_data)
			;
		++old_data;
		ret = efi_var_mem_ins(variable_name, vendor,
				      attributes & ~EFI_VARIABLE_APPEND_WRITE,
				      var->length, old_data, data_size, data,
				      time);
	} else {
		ret = efi_var_mem_ins(variable_name, vendor, attributes,
				      data_size, data, 0, NULL, time);
	}

	if (ret != EFI_SUCCESS)
		return ret;

	efi_var_mem_del(var);

	if (var_type == EFI_AUTH_VAR_PK)
		ret = efi_init_secure_state();
	else
		ret = EFI_SUCCESS;

	/*
	 * Write non-volatile EFI variables to file
	 * TODO: check if a value change has occured to avoid superfluous writes
	 */
	if (attributes & EFI_VARIABLE_NON_VOLATILE)
		efi_var_to_file();

	return EFI_SUCCESS;
}

/**
 * efi_init_variables() - initialize variable services
 *
 * Return:	status code
 */
static efi_status_t efi_init_variables(void *data)
{
	efi_status_t ret;

	ret = efi_var_mem_init();
	if (ret != EFI_SUCCESS)
		return ret;

	ret = efi_var_from_file(AT_FDCWD, BAREBOX_ENV_INIT_EFIVARS);
	if (ret != EFI_SUCCESS && ret != EFI_NOT_FOUND) {
		pr_err(BAREBOX_ENV_INIT_EFIVARS
		       " : Invalid EFI variables file (%lx)\n", ret);
		return ret;
	}

	return efi_init_secure_state();
}

static int efi_variables_init(void)
{
	efi_register_deferred_init(efi_init_variables, NULL);
	return 0;
}
postcore_initcall(efi_variables_init);

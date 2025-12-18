/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/89d911e386cc439c68ab961e76937c377e7d7127/include/efi_variable.h */
/*
 * Copyright (c) 2020, Heinrich Schuchardt <xypron.glpk@gmx.de>
 */
#ifndef _EFI_RUNTIME_H
#define _EFI_RUNTIME_H

#include <efi/types.h>

#define EFI_VARIABLE_READ_ONLY 0x80000000

/**
 * efi_get_variable_int() - retrieve value of a UEFI variable
 *
 * @variable_name:	name of the variable
 * @vendor:		vendor GUID
 * @attributes:		attributes of the variable
 * @data_size:		size of the buffer to which the variable value is copied
 * @data:		buffer to which the variable value is copied
 * @timep:		authentication time (seconds since start of epoch)
 * Return:		status code
 */
efi_status_t __efi_runtime
efi_get_variable_int(const u16 *variable_name,
		     const efi_guid_t *vendor,
		     u32 *attributes, efi_uintn_t *data_size,
		     void *data, u64 *timep);

/**
 * efi_set_variable_int() - set value of a UEFI variable
 *
 * @variable_name:	name of the variable
 * @vendor:		vendor GUID
 * @attributes:		attributes of the variable
 * @data_size:		size of the buffer with the variable value
 * @data:		buffer with the variable value
 * @ro_check:		check the read only read only bit in attributes
 * Return:		status code
 */
efi_status_t efi_set_variable_int(const u16 *variable_name,
				  const efi_guid_t *vendor,
				  u32 attributes, efi_uintn_t data_size,
				  const void *data, bool ro_check);

/**
 * efi_query_variable_info_int() - get information about EFI variables
 *
 * This function implements the QueryVariableInfo() runtime service.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
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
efi_status_t __efi_runtime
efi_query_variable_info_int(u32 attributes,
			    u64 *maximum_variable_storage_size,
			    u64 *remaining_variable_storage_size,
			    u64 *maximum_variable_size);

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
			u32 attributes, efi_uintn_t data_size, const void *data);


efi_status_t __efi_runtime EFIAPI
efirt_set_variable(u16 *variable_name, const efi_guid_t *vendor,
		   u32 attributes, efi_uintn_t data_size,
		   const void *data);

/**
 * efi_var_mem_find() - find a variable in the list
 *
 * @guid:	GUID of the variable
 * @name:	name of the variable
 * @next:	on exit pointer to the next variable after the found one
 * Return:	found variable
 */
struct efi_var_entry *efi_var_mem_find(const efi_guid_t *guid, const u16 *name,
				       struct efi_var_entry **next);

/**
 * efi_var_mem_ins() - append a variable to the list of variables
 *
 * The variable is appended without checking if a variable of the same name
 * already exists. The two data buffers are concatenated.
 *
 * @variable_name:	variable name
 * @vendor:		GUID
 * @attributes:		variable attributes
 * @size1:		size of the first data buffer
 * @data1:		first data buffer
 * @size2:		size of the second data field
 * @data2:		second data buffer
 * @time:		time of authentication (as seconds since start of epoch)
 * Result:		status code
 */
efi_status_t __efi_runtime
efi_var_mem_ins(const u16 *variable_name,
		const efi_guid_t *vendor, u32 attributes,
		const efi_uintn_t size1, const void *data1,
		const efi_uintn_t size2, const void *data2,
		const u64 time);

/**
 * efi_get_next_variable_name_int() - enumerate the current variable names
 *
 * @variable_name_size:	size of variable_name buffer in byte
 * @variable_name:	name of uefi variable's name in u16
 * @vendor:		vendor's guid
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * Return: status code
 */
efi_status_t __efi_runtime
efi_get_next_variable_name_int(efi_uintn_t *variable_name_size,
			       u16 *variable_name,
			       efi_guid_t *vendor);

/**
 * efi_var_mem_del() - delete a variable from the list of variables
 *
 * @var:	variable to delete
 */
void __efi_runtime efi_var_mem_del(struct efi_var_entry *var);

/**
 * efi_var_mem_free() - determine free memory for variables
 *
 * Return:	maximum data size plus variable name size
 */
u64 __efi_runtime efi_var_mem_free(void);

struct efi_var_file;
extern __efi_runtime void efi_var_buf_set(struct efi_var_file *buf, unsigned size);
extern __efi_runtime struct efi_var_file *efi_var_buf_get(unsigned *size);

struct efi_system_table;
void __efi_runtime efirt_trace_init(void *_ring);
void efi_runtime_prep(void);
void __efi_runtime efi_runtime_detach(struct efi_system_table *systab);

#endif

/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/89d911e386cc439c68ab961e76937c377e7d7127/include/efi_variable.h */

#ifndef _EFI_LOADER_VARIABLE_H
#define _EFI_LOADER_VARIABLE_H

#include <efi/types.h>
#include <efi/error.h>

#define EFI_VARIABLE_READ_ONLY 0x80000000

enum efi_auth_var_type {
	EFI_AUTH_VAR_NONE = 0,
	EFI_AUTH_MODE,
	EFI_AUTH_VAR_PK,
	EFI_AUTH_VAR_KEK,
	EFI_AUTH_VAR_DB,
	EFI_AUTH_VAR_DBX,
	EFI_AUTH_VAR_DBT,
	EFI_AUTH_VAR_DBR,
};

/**
 * efi_auth_var_get_type() - convert variable name and guid to enum
 *
 * @name:	name of UEFI variable
 * @guid:	guid of UEFI variable
 * Return:	identifier for authentication related variables
 */
enum efi_auth_var_type efi_auth_var_get_type(const u16 *name,
					     const efi_guid_t *guid);

/**
 * efi_auth_var_get_guid() - get the predefined GUID for a variable name
 *
 * @name:	name of UEFI variable
 * Return:	guid of UEFI variable
 */
const efi_guid_t *efi_auth_var_get_guid(const u16 *name);

/**
 * efi_var_mem_init() - set-up variable list
 *
 * Return:	status code
 */
efi_status_t efi_var_mem_init(void);

/**
 * efi_get_next_variable_name_mem() - Runtime common code across efi variable
 *                                    implementations for GetNextVariable()
 *                                    from the cached memory copy
 *
 * @variable_name_size:	size of variable_name buffer in bytes
 * @variable_name:	name of uefi variable's name in u16
 * @mask:		bitmask with required attributes of variables to be collected.
 *                      variables are only collected if all of the required
 *                      attributes match. Use 0 to skip matching
 * @vendor:		vendor's guid
 *
 * Return: status code
 */
efi_status_t __efi_runtime
efi_get_next_variable_name_mem(efi_uintn_t *variable_name_size, u16 *variable_name,
			       efi_guid_t *vendor, u32 mask);
/**
 * efi_get_variable_mem() - Runtime common code across efi variable
 *                          implementations for GetVariable() from
 *                          the cached memory copy
 *
 * @variable_name:	name of the variable
 * @vendor:		vendor GUID
 * @attributes:		attributes of the variable
 * @data_size:		size of the buffer to which the variable value is copied
 * @data:		buffer to which the variable value is copied
 * @timep:		authentication time (seconds since start of epoch)
 * @mask:		bitmask with required attributes of variables to be collected.
 *                      variables are only collected if all of the required
 *                      attributes match. Use 0 to skip matching
 * Return:		status code
 */
efi_status_t __efi_runtime
efi_get_variable_mem(const u16 *variable_name, const efi_guid_t *vendor,
		     u32 *attributes, efi_uintn_t *data_size, void *data,
		     u64 *timep, u32 mask);

/*
 * This constant identifies the file format for storing UEFI variables in
 * struct efi_var_file.
 */
#define EFI_VAR_FILE_MAGIC 0x0161566966456255 /* UbEfiVa, version 1 */

/**
 * struct efi_var_entry - UEFI variable file entry
 *
 * @length:	length of enty, multiple of 8
 * @attr:	variable attributes
 * @time:	authentication time (seconds since start of epoch)
 * @guid:	vendor GUID
 * @name:	UTF16 variable name
 */
struct efi_var_entry {
	u32 length;
	u32 attr;
	u64 time;
	efi_guid_t guid;
	u16 name[];
};

/**
 * struct efi_var_file - file for storing UEFI variables
 *
 * @reserved:	unused, may be overwritten by memory probing
 * @magic:	identifies file format, takes value %EFI_VAR_FILE_MAGIC
 * @length:	length including header
 * @crc32:	CRC32 without header
 * @var:	variables
 */
struct efi_var_file {
	u64 reserved;
	u64 magic;
	u32 length;
	u32 crc32;
	struct efi_var_entry var[];
};

/**
 * efi_var_buf_update() - udpate memory buffer for variables
 *
 * @var_buf:	source buffer
 *
 * This function copies to the memory buffer for UEFI variables. Call this
 * function in ExitBootServices() if memory backed variables are only used
 * at runtime to fill the buffer.
 */
void efi_var_buf_update(struct efi_var_file *var_buf);

/* The name within the ESP of the file used to store barebox' EFI variables */
extern char *efi_var_file_name;

/**
 * efi_var_from_file() - read variables from file
 *
 * File ubootefi.var is read from the EFI system partitions and the variables
 * stored in the file are created.
 *
 * In case the file does not exist yet or a variable cannot be set EFI_SUCCESS
 * is returned.
 *
 * Return:	status code
 */
efi_status_t efi_var_from_file(int dirfd, const char *filename);

#endif

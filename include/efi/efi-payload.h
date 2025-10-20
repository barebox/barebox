/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_PAYLOAD_H
#define __EFI_PAYLOAD_H

#include <efi/types.h>
#include <efi/efi-util.h>
#include <efi/memtype.h>

struct efi_system_table;
struct efi_loaded_image;

struct barebox_efi_data {
	void *image;
	void *sys_table;
};

extern struct efi_system_table *efi_sys_table;
extern efi_handle_t efi_parent_image;
extern struct efi_device_path *efi_device_path;
extern struct efi_loaded_image *efi_loaded_image;

void *efi_get_variable(char *name, efi_guid_t *vendor, int *var_size);

static inline void *efi_get_global_var(char *name, int *var_size)
{
	extern efi_guid_t efi_global_variable_guid;
	return efi_get_variable(name, &efi_global_variable_guid, var_size);
}

int efi_set_variable(char *name, efi_guid_t *vendor, uint32_t attributes,
		     void *buf, size_t size);
int efi_set_variable_usec(char *name, efi_guid_t *vendor, uint64_t usec);

void *efi_earlymem_alloc(const struct efi_system_table *sys_table,
			 size_t memsize, enum efi_memory_type mem_type);

int efi_initrd_register(void *initrd, size_t initrd_size);
void efi_initrd_unregister(void);

#endif

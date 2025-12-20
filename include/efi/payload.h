/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PAYLOAD_H
#define __EFI_PAYLOAD_H

#include <efi/types.h>
#include <efi/memory.h>
#include <efi/services.h>

struct efi_system_table;
struct efi_loaded_image;
struct efi_boot_services;
struct efi_runtime_services;

struct barebox_efi_data {
	void *image;
	void *sys_table;
};

extern struct efi_boot_services *BS;
extern struct efi_runtime_services *RT;
extern struct efi_system_table *efi_sys_table;
extern efi_handle_t efi_parent_image;
extern struct efi_device_path *efi_device_path;
extern struct efi_loaded_image *efi_loaded_image;

void *efi_earlymem_alloc(const struct efi_system_table *sys_table,
			 size_t memsize, enum efi_memory_type mem_type);

__attribute__((noreturn)) void efi_main(efi_handle_t, struct efi_system_table *);

#define for_each_efi_config_table(t) \
	for (t = efi_sys_table->tables; \
	     t - efi_sys_table->tables < efi_sys_table->nr_tables; \
	     t++)

#endif

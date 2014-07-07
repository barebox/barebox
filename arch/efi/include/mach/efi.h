#ifndef __MACH_EFI_H
#define __MACH_EFI_H

#include <efi.h>

const char *efi_strerror(efi_status_t err);

extern efi_system_table_t *efi_sys_table;
extern efi_handle_t efi_parent_image;
extern struct efi_device_path *efi_device_path;
extern efi_loaded_image_t *efi_loaded_image;

int efi_errno(efi_status_t err);

int efi_clocksource_init(void);

void *efi_get_variable(char *name, efi_guid_t *vendor, int *var_size);

static inline void *efi_get_global_var(char *name, int *var_size)
{
	return efi_get_variable(name, &efi_global_variable_guid, var_size);
}

#endif /* __MACH_EFI_H */

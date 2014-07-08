/*
 * efi.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <sizes.h>
#include <memory.h>
#include <clock.h>
#include <command.h>
#include <magicvar.h>
#include <init.h>
#include <driver.h>
#include <ns16550.h>
#include <io.h>
#include <efi.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <boot.h>
#include <fs.h>
#include <binfmt.h>
#include <wchar.h>
#include <envfs.h>
#include <efi.h>
#include <mach/efi.h>
#include <mach/efi-device.h>

efi_runtime_services_t *RT;
efi_boot_services_t *BS;
efi_system_table_t *efi_sys_table;
efi_handle_t efi_parent_image;
struct efi_device_path *efi_device_path;
efi_loaded_image_t *efi_loaded_image;

void *efi_get_variable(char *name, efi_guid_t *vendor, int *var_size)
{
	efi_status_t efiret;
	void *buf;
	unsigned long size = 0;
	s16 *name16 = strdup_char_to_wchar(name);

	efiret = RT->get_variable(name16, vendor, NULL, &size, NULL);

	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL) {
		buf = ERR_PTR(-efi_errno(efiret));
		goto out;
	}

	buf = malloc(size);
	if (!buf) {
		buf = ERR_PTR(-ENOMEM);
		goto out;
	}

	efiret = RT->get_variable(name16, vendor, NULL, &size, buf);
	if (EFI_ERROR(efiret)) {
		free(buf);
		buf = ERR_PTR(-efi_errno(efiret));
		goto out;
	}

	if (var_size)
		*var_size = size;

out:
	free(name16);

	return buf;
}

struct efi_boot {
	u32 attributes;
	u16 file_path_len;
	char *description;
	struct efi_device_path *path;
	void *binary;
};

struct efi_boot *efi_get_boot(int num)
{
	struct efi_boot *boot = xzalloc(sizeof(*boot));
	void *buf, *ptr;
	int size;
	char *name;

	name = asprintf("Boot%04X", num);

	buf = efi_get_global_var(name, &size);

	free(name);

	if (!buf) {
		free(boot);
		return NULL;
	}

	ptr = buf;

	boot->attributes = *(u32 *)ptr;

	ptr += sizeof(u32);

	boot->file_path_len = *(u16 *)ptr;

	ptr += sizeof(u16);

	boot->description = strdup_wchar_to_char(ptr);

	ptr += (strlen(boot->description) + 1) * 2;

	printf("description: %s\n", boot->description);

	boot->path = memdup(ptr, boot->file_path_len);

	printf("path: %s\n", device_path_to_str(boot->path));

	return boot;
}

static int misc_init(void)
{
	efi_get_boot(1);
	efi_get_boot(2);
	efi_get_boot(3);

	return 0;
}
late_initcall(misc_init);

const char *efi_strerror(efi_status_t err)
{
	const char *str;

	switch (err) {
	case EFI_SUCCESS: str = "Success"; break;
	case EFI_LOAD_ERROR: str = "Load Error"; break;
	case EFI_INVALID_PARAMETER: str = "Invalid Parameter"; break;
	case EFI_UNSUPPORTED: str = "Unsupported"; break;
	case EFI_BAD_BUFFER_SIZE: str = "Bad Buffer Size"; break;
	case EFI_BUFFER_TOO_SMALL: str = "Buffer Too Small"; break;
	case EFI_NOT_READY: str = "Not Ready"; break;
	case EFI_DEVICE_ERROR: str = "Device Error"; break;
	case EFI_WRITE_PROTECTED: str = "Write Protected"; break;
	case EFI_OUT_OF_RESOURCES: str = "Out of Resources"; break;
	case EFI_VOLUME_CORRUPTED: str = "Volume Corrupt"; break;
	case EFI_VOLUME_FULL: str = "Volume Full"; break;
	case EFI_NO_MEDIA: str = "No Media"; break;
	case EFI_MEDIA_CHANGED: str = "Media changed"; break;
	case EFI_NOT_FOUND: str = "Not Found"; break;
	case EFI_ACCESS_DENIED: str = "Access Denied"; break;
	case EFI_NO_RESPONSE: str = "No Response"; break;
	case EFI_NO_MAPPING: str = "No mapping"; break;
	case EFI_TIMEOUT: str = "Time out"; break;
	case EFI_NOT_STARTED: str = "Not started"; break;
	case EFI_ALREADY_STARTED: str = "Already started"; break;
	case EFI_ABORTED: str = "Aborted"; break;
	case EFI_ICMP_ERROR: str = "ICMP Error"; break;
	case EFI_TFTP_ERROR: str = "TFTP Error"; break;
	case EFI_PROTOCOL_ERROR: str = "Protocol Error"; break;
	case EFI_INCOMPATIBLE_VERSION: str = "Incompatible Version"; break;
	case EFI_SECURITY_VIOLATION: str = "Security Violation"; break;
	case EFI_CRC_ERROR: str = "CRC Error"; break;
	case EFI_END_OF_MEDIA: str = "End of Media"; break;
	case EFI_END_OF_FILE: str = "End of File"; break;
	case EFI_INVALID_LANGUAGE: str = "Invalid Language"; break;
	case EFI_COMPROMISED_DATA: str = "Compromised Data"; break;
	default: str = "unknown error";
	}

	return str;
}

int efi_errno(efi_status_t err)
{
	int ret;

	switch (err) {
	case EFI_SUCCESS: ret = 0; break;
	case EFI_LOAD_ERROR: ret = EIO; break;
	case EFI_INVALID_PARAMETER: ret = EINVAL; break;
	case EFI_UNSUPPORTED: ret = ENOTSUPP; break;
	case EFI_BAD_BUFFER_SIZE: ret = EINVAL; break;
	case EFI_BUFFER_TOO_SMALL: ret = EINVAL; break;
	case EFI_NOT_READY: ret = EAGAIN; break;
	case EFI_DEVICE_ERROR: ret = EIO; break;
	case EFI_WRITE_PROTECTED: ret = EROFS; break;
	case EFI_OUT_OF_RESOURCES: ret = ENOMEM; break;
	case EFI_VOLUME_CORRUPTED: ret = EIO; break;
	case EFI_VOLUME_FULL: ret = ENOSPC; break;
	case EFI_NO_MEDIA: ret = ENOMEDIUM; break;
	case EFI_MEDIA_CHANGED: ret = ENOMEDIUM; break;
	case EFI_NOT_FOUND: ret = ENODEV; break;
	case EFI_ACCESS_DENIED: ret = EACCES; break;
	case EFI_NO_RESPONSE: ret = ETIMEDOUT; break;
	case EFI_NO_MAPPING: ret = EINVAL; break;
	case EFI_TIMEOUT: ret = ETIMEDOUT; break;
	case EFI_NOT_STARTED: ret = EINVAL; break;
	case EFI_ALREADY_STARTED: ret = EINVAL; break;
	case EFI_ABORTED: ret = EINTR; break;
	case EFI_ICMP_ERROR: ret = EINVAL; break;
	case EFI_TFTP_ERROR: ret = EINVAL; break;
	case EFI_PROTOCOL_ERROR: ret = EPROTO; break;
	case EFI_INCOMPATIBLE_VERSION: ret = EINVAL; break;
	case EFI_SECURITY_VIOLATION: ret = EINVAL; break;
	case EFI_CRC_ERROR: ret = EINVAL; break;
	case EFI_END_OF_MEDIA: ret = EINVAL; break;
	case EFI_END_OF_FILE: ret = EINVAL; break;
	case EFI_INVALID_LANGUAGE: ret = EINVAL; break;
	case EFI_COMPROMISED_DATA: ret = EINVAL; break;
	default: ret = EINVAL;
	}

	return ret;
}

static struct NS16550_plat ns16550_plat = {
	.clock = 115200 * 16,
};

static int efi_console_init(void)
{
	barebox_set_model("barebox EFI payload");

	add_generic_device("efi-stdio", DEVICE_ID_SINGLE, NULL, 0 , 0, 0, NULL);

	if (IS_ENABLED(CONFIG_ARCH_EFI_REGISTER_COM1))
		add_ns16550_device(0, 0x3f8, 0x10, IORESOURCE_IO | IORESOURCE_MEM_8BIT,
				&ns16550_plat);

	return 0;
}
console_initcall(efi_console_init);

void reset_cpu(unsigned long addr)
{
	BS->exit(efi_parent_image, EFI_SUCCESS, 0, NULL);

	while(1);
}

extern char image_base[];
extern initcall_t __barebox_initcalls_start[], __barebox_early_initcalls_end[],
		  __barebox_initcalls_end[];

/*
 * We have a position independent binary generated with -fpic. This function
 * fixes the linker generated tables.
 */
static void fixup_tables(void)
{
	initcall_t *initcall;
	unsigned long offset = (unsigned long)image_base;
	struct command *cmdtp;
	struct magicvar *m;

	for (initcall = __barebox_initcalls_start;
			initcall < __barebox_initcalls_end; initcall++)
		*initcall += offset;

	for (cmdtp = &__barebox_cmd_start;
			cmdtp != &__barebox_cmd_end;
			cmdtp++) {
		cmdtp->name += offset;
		cmdtp->cmd += offset;
		if (cmdtp->complete)
			cmdtp->complete += offset;
		if (cmdtp->desc)
			cmdtp->desc += offset;
		if (cmdtp->help)
			cmdtp->help += offset;
		if (cmdtp->opts)
			cmdtp->opts += offset;
		if (cmdtp->aliases)
			cmdtp->aliases = (void *)cmdtp->aliases + offset;
	}

	for (m = &__barebox_magicvar_start;
			m != &__barebox_magicvar_end;
			m++) {
		m->name += offset;
		m->description += offset;
	}
}

static int efi_init(void)
{
	defaultenv_append_directory(env_efi);

	return 0;
}
device_initcall(efi_init);

/**
 * efi-main - Entry point for EFI images
 */
efi_status_t efi_main(efi_handle_t image, efi_system_table_t *sys_table)
{
	void *mem;
	efi_status_t efiret;

#ifdef DEBUG
	sys_table->con_out->output_string(sys_table->con_out, L"barebox\n");
#endif

	BS = sys_table->boottime;

	efi_parent_image = image;
	efi_sys_table = sys_table;
	RT = sys_table->runtime;

	efiret = BS->open_protocol(efi_parent_image, &efi_loaded_image_protocol_guid,
			(void **)&efi_loaded_image,
			efi_parent_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (!EFI_ERROR(efiret))
		BS->handle_protocol(efi_loaded_image->device_handle,
				&efi_device_path_protocol_guid, (void **)&efi_device_path);

	fixup_tables();

	BS->allocate_pool(efi_loaded_image->image_data_type, SZ_16M, &mem);
	mem_malloc_init(mem, mem + SZ_16M);

	efi_clocksource_init();

	start_barebox();

	return EFI_SUCCESS;
}

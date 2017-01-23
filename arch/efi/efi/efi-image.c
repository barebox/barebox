/*
 * efi-image.c - barebox EFI payload support
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

#include <clock.h>
#include <common.h>
#include <linux/sizes.h>
#include <memory.h>
#include <command.h>
#include <magicvar.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <efi.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <boot.h>
#include <bootm.h>
#include <fs.h>
#include <libfile.h>
#include <binfmt.h>
#include <wchar.h>
#include <mach/efi.h>
#include <mach/efi-device.h>

struct linux_kernel_header {
	/* first sector of the image */
	uint8_t code1[0x0020];
	uint16_t cl_magic;		/**< Magic number 0xA33F */
	uint16_t cl_offset;		/**< The offset of command line */
	uint8_t code2[0x01F1 - 0x0020 - 2 - 2];
	uint8_t setup_sects;		/**< The size of the setup in sectors */
	uint16_t root_flags;		/**< If the root is mounted readonly */
	uint16_t syssize;		/**< obsolete */
	uint16_t swap_dev;		/**< obsolete */
	uint16_t ram_size;		/**< obsolete */
	uint16_t vid_mode;		/**< Video mode control */
	uint16_t root_dev;		/**< Default root device number */
	uint16_t boot_flag;		/**< 0xAA55 magic number */

	/* second sector of the image */
	uint16_t jump;			/**< Jump instruction (this is code!) */
	uint32_t header;		/**< Magic signature "HdrS" */
	uint16_t version;		/**< Boot protocol version supported */
	uint32_t realmode_swtch;	/**< Boot loader hook */
	uint16_t start_sys;		/**< The load-low segment (obsolete) */
	uint16_t kernel_version;	/**< Points to kernel version string */
	uint8_t type_of_loader;		/**< Boot loader identifier */
	uint8_t loadflags;		/**< Boot protocol option flags */
	uint16_t setup_move_size;	/**< Move to high memory size */
	uint32_t code32_start;		/**< Boot loader hook */
	uint32_t ramdisk_image;		/**< initrd load address */
	uint32_t ramdisk_size;		/**< initrd size */
	uint32_t bootsect_kludge;	/**< obsolete */
	uint16_t heap_end_ptr;		/**< Free memory after setup end */
	uint8_t ext_loader_ver;		/**< boot loader's extension of the version number */
	uint8_t ext_loader_type;	/**< boot loader's extension of its type */
	uint32_t cmd_line_ptr;		/**< Points to the kernel command line */
	uint32_t initrd_addr_max;	/**< Highest address for initrd */
	uint32_t kernel_alignment;	/**< Alignment unit required by the kernel */
	uint8_t relocatable_kernel;	/** */
	uint8_t min_alignment;		/** */
	uint16_t xloadflags;		/** */
	uint32_t cmdline_size;		/** */
	uint32_t hardware_subarch;	/** */
	uint64_t hardware_subarch_data;	/** */
	uint32_t payload_offset;	/** */
	uint32_t payload_length;	/** */
	uint64_t setup_data;		/** */
	uint64_t pref_address;		/** */
	uint32_t init_size;		/** */
	uint32_t handover_offset;	/** */
} __attribute__ ((packed));

int efi_load_image(const char *file, efi_loaded_image_t **loaded_image,
		efi_handle_t *h)
{
	void *exe;
	size_t size;
	efi_handle_t handle;
	efi_status_t efiret = EFI_SUCCESS;

	exe = read_file(file, &size);
	if (!exe)
		return -EINVAL;

	efiret = BS->load_image(false, efi_parent_image, efi_device_path, exe, size,
			&handle);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to LoadImage: %s\n", efi_strerror(efiret));
		goto out;
	};

	efiret = BS->open_protocol(handle, &efi_loaded_image_protocol_guid,
			(void **)loaded_image,
			efi_parent_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret)) {
		pr_err("failed to OpenProtocol: %s\n", efi_strerror(efiret));
		BS->unload_image(handle);
		goto out;
	}

	*h = handle;
out:
	memset(exe, 0, size);
	free(exe);
	return -efi_errno(efiret);
}

static int efi_execute_image(const char *file)
{
	efi_handle_t handle;
	efi_loaded_image_t *loaded_image;
	efi_status_t efiret;
	struct linux_kernel_header *image_header;
	const char *options;
	bool is_driver;
	int ret;

	ret = efi_load_image(file, &loaded_image, &handle);
	if (ret)
		return ret;

	is_driver = (loaded_image->image_code_type == EFI_BOOT_SERVICES_CODE) ||
		(loaded_image->image_code_type == EFI_RUNTIME_SERVICES_CODE);

	image_header = (struct linux_kernel_header *)loaded_image->image_base;
	if (image_header->boot_flag == 0xAA55 &&
	    image_header->header == 0x53726448) {
		pr_debug("Linux kernel detected. Adding bootargs.");
		options = linux_bootargs_get();
		pr_err("add linux options '%s'\n", options);
		loaded_image->load_options = xstrdup_char_to_wchar(options);
		loaded_image->load_options_size =
			(strlen(options) + 1) * sizeof(wchar_t);
		shutdown_barebox();
	}

	efiret = BS->start_image(handle, NULL, NULL);
	if (EFI_ERROR(efiret))
		pr_err("failed to StartImage: %s\n", efi_strerror(efiret));

	if (!is_driver)
		BS->unload_image(handle);

	efi_connect_all();
	efi_register_devices();

	return -efi_errno(efiret);
}

#ifdef __x86_64__
typedef void(*handover_fn)(void *image, efi_system_table_t *table,
		struct linux_kernel_header *header);

static inline void linux_efi_handover(efi_handle_t handle,
		struct linux_kernel_header *header)
{
	handover_fn handover;

	asm volatile ("cli");
	handover = (handover_fn)((long)header->code32_start + 512 +
				 header->handover_offset);
	handover(handle, efi_sys_table, header);
}
#else
typedef void(*handover_fn)(VOID *image, EFI_SYSTEM_TABLE *table,
		struct SetupHeader *setup) __attribute__((regparm(0)));

static inline void linux_efi_handover(efi_handle_t handle,
		struct linux_kernel_header *header)
{
	handover_fn handover;

	handover = (handover_fn)((long)header->code32_start +
				 header->handover_offset);
	handover(handle, efi_sys_table, header);
}
#endif

static int do_bootm_efi(struct image_data *data)
{
	void *tmp;
	void *initrd = NULL;
	size_t size;
	efi_handle_t handle;
	int ret;
	const char *options;
	efi_loaded_image_t *loaded_image;
	struct linux_kernel_header *image_header, *boot_header;

	ret = efi_load_image(data->os_file, &loaded_image, &handle);
	if (ret)
		return ret;

	image_header = (struct linux_kernel_header *)loaded_image->image_base;

	if (image_header->boot_flag != 0xAA55 ||
	    image_header->header != 0x53726448 ||
	    image_header->version < 0x20b ||
	    !image_header->relocatable_kernel) {
		pr_err("Not a valid kernel image!\n");
		BS->unload_image(handle);
		return -EINVAL;
	}

	boot_header = xmalloc(0x4000);
	memset(boot_header, 0, 0x4000);
	memcpy(boot_header, image_header, sizeof(*image_header));

	boot_header->type_of_loader = 0xff;

	if (data->initrd_file) {
		tmp = read_file(data->initrd_file, &size);
		initrd = xmemalign(PAGE_SIZE, PAGE_ALIGN(size));
		memcpy(initrd, tmp, size);
		memset(initrd + size, 0, PAGE_ALIGN(size) - size);
		free(tmp);
		boot_header->ramdisk_image = (uint64_t)initrd;
		boot_header->ramdisk_size = PAGE_ALIGN(size);
	}

	options = linux_bootargs_get();
	boot_header->cmd_line_ptr = (uint64_t)options;
	boot_header->cmdline_size = strlen(options);

	boot_header->code32_start = (uint64_t)loaded_image->image_base +
			(image_header->setup_sects+1) * 512;

	if (bootm_verbose(data)) {
		printf("\nStarting kernel at 0x%p", loaded_image->image_base);
		if (data->initrd_file)
			printf(", initrd at 0x%08x",
			       boot_header->ramdisk_image);
		printf("...\n");
	}

	if (data->dryrun) {
		BS->unload_image(handle);
		free(boot_header);
		free(initrd);
		return 0;
	}

	efi_set_variable_usec("LoaderTimeExecUSec", &efi_systemd_vendor_guid,
			      get_time_ns()/1000);

	shutdown_barebox();
	linux_efi_handover(handle, boot_header);

	return 0;
}

static struct image_handler efi_handle_tr = {
	.name = "EFI Application",
	.bootm = do_bootm_efi,
	.filetype = filetype_exe,
};

static int efi_execute(struct binfmt_hook *b, char *file, int argc, char **argv)
{
	return efi_execute_image(file);
}

static struct binfmt_hook binfmt_efi_hook = {
	.type = filetype_exe,
	.hook = efi_execute,
};

static int efi_register_image_handler(void)
{
	register_image_handler(&efi_handle_tr);
	binfmt_register(&binfmt_efi_hook);

	return 0;
}
late_initcall(efi_register_image_handler);

// SPDX-License-Identifier: GPL-2.0-only
/*
 * image.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <efi/efi-payload.h>
#include <efi/efi-device.h>

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

static void *efi_read_file(const char *file, size_t *size)
{
	efi_physical_addr_t mem;
	efi_status_t efiret;
	struct stat s;
	char *buf;
	ssize_t ret;

	buf = read_file(file, size);
	if (buf || errno != ENOMEM)
		return buf;

	ret = stat(file, &s);
	if (ret)
		return NULL;

	efiret = BS->allocate_pages(EFI_ALLOCATE_ANY_PAGES,
				    EFI_LOADER_CODE,
				    DIV_ROUND_UP(s.st_size, EFI_PAGE_SIZE),
				    &mem);
	if (EFI_ERROR(efiret)) {
		errno = efi_errno(efiret);
		return NULL;
	}

	buf = (void *)mem;

	ret = read_file_into_buf(file, buf, s.st_size);
	if (ret < 0)
		return NULL;

	*size = ret;
	return buf;
}

static void efi_free_file(void *_mem, size_t size)
{
	efi_physical_addr_t mem = (efi_physical_addr_t)_mem;

	if (mem_malloc_start() <= mem && mem < mem_malloc_end())
		free(_mem);
	else
		BS->free_pages(mem, DIV_ROUND_UP(size, EFI_PAGE_SIZE));
}

static int efi_load_image(const char *file, struct efi_loaded_image **loaded_image,
		efi_handle_t *h)
{
	void *exe;
	size_t size;
	efi_handle_t handle;
	efi_status_t efiret = EFI_SUCCESS;

	exe = efi_read_file(file, &size);
	if (!exe)
		return -errno;

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
	efi_free_file(exe, size);
	return -efi_errno(efiret);
}

static bool is_linux_image(enum filetype filetype, const void *base)
{
	const struct linux_kernel_header *hdr = base;

	if (IS_ENABLED(CONFIG_X86) &&
	    hdr->boot_flag == 0xAA55 && hdr->header == 0x53726448)
		return true;

	if (IS_ENABLED(CONFIG_CPU_V8) &&
	    filetype == filetype_arm64_efi_linux_image)
		return true;

	return false;
}

static int efi_execute_image(enum filetype filetype, const char *file)
{
	efi_handle_t handle;
	struct efi_loaded_image *loaded_image;
	efi_status_t efiret;
	const char *options;
	bool is_driver;
	int ret;

	ret = efi_load_image(file, &loaded_image, &handle);
	if (ret)
		return ret;

	is_driver = (loaded_image->image_code_type == EFI_BOOT_SERVICES_CODE) ||
		(loaded_image->image_code_type == EFI_RUNTIME_SERVICES_CODE);

	if (is_linux_image(filetype, loaded_image->image_base)) {
		pr_debug("Linux kernel detected. Adding bootargs.");
		options = linux_bootargs_get();
		pr_info("add linux options '%s'\n", options);
		if (options) {
			loaded_image->load_options = xstrdup_char_to_wchar(options);
			loaded_image->load_options_size =
				(strlen(options) + 1) * sizeof(wchar_t);
		}
		shutdown_barebox();
	}

	efi_pause_devices();

	efiret = BS->start_image(handle, NULL, NULL);
	if (EFI_ERROR(efiret))
		pr_err("failed to StartImage: %s\n", efi_strerror(efiret));

	efi_continue_devices();

	if (!is_driver)
		BS->unload_image(handle);

	efi_connect_all();
	efi_register_devices();

	return -efi_errno(efiret);
}

#ifdef __x86_64__
typedef void(*handover_fn)(void *image, struct efi_system_table *table,
		struct linux_kernel_header *header);

static inline void linux_efi_handover(efi_handle_t handle,
		struct linux_kernel_header *header)
{
	handover_fn handover;

	handover = (handover_fn)((long)header->code32_start + 512 +
				 header->handover_offset);
	handover(handle, efi_sys_table, header);
}
#else
typedef void(*handover_fn)(void *image, struct efi_system_table *table,
		struct linux_kernel_header *setup);

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
	struct efi_loaded_image *loaded_image;
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

	/* Refer to Linux kernel commit a27e292b8a54
	 * ("Documentation/x86/boot: Reserve type_of_loader=13 for barebox")
	 */
	boot_header->type_of_loader = 0x13;

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
	if (options) {
		boot_header->cmd_line_ptr = (uint64_t)options;
		boot_header->cmdline_size = strlen(options);
	}

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
	return efi_execute_image(b->type, file);
}

static struct binfmt_hook binfmt_efi_hook = {
	.type = filetype_exe,
	.hook = efi_execute,
};

static struct binfmt_hook binfmt_arm64_efi_hook = {
	.type = filetype_arm64_efi_linux_image,
	.hook = efi_execute,
};

static int efi_register_image_handler(void)
{
	register_image_handler(&efi_handle_tr);
	binfmt_register(&binfmt_efi_hook);

	if (IS_ENABLED(CONFIG_CPU_V8))
		binfmt_register(&binfmt_arm64_efi_hook);

	return 0;
}
late_efi_initcall(efi_register_image_handler);

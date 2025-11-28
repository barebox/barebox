// SPDX-License-Identifier: GPL-2.0-only
/*
 * handover.c - legacy x86 EFI handover protocol
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#define pr_fmt(fmt) "efi-handover: " fmt

#include <clock.h>
#include <common.h>
#include <linux/sizes.h>
#include <linux/ktime.h>
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

#include "image.h"

struct x86_setup_header {
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

static inline bool is_x86_setup_header(const void *base)
{
	const struct x86_setup_header *hdr = base;

	return hdr->boot_flag == 0xAA55 && hdr->header == 0x53726448;
}

typedef void(*handover_fn)(void *image, struct efi_system_table *table,
			   struct x86_setup_header *header);

static inline void linux_efi_handover(efi_handle_t handle,
				      struct x86_setup_header *header)
{
	handover_fn handover;
	uintptr_t addr;

	addr = header->code32_start + header->handover_offset;
	if (IS_ENABLED(CONFIG_X86_64))
		addr += 512;

	handover = efi_phys_to_virt(addr);
	handover(handle, efi_sys_table, header);
}

static int do_bootm_efi(struct image_data *data)
{
	void *tmp;
	void *initrd = NULL;
	size_t size;
	efi_handle_t handle;
	int ret;
	const char *options;
	struct efi_loaded_image *loaded_image;
	struct x86_setup_header *image_header, *boot_header;

	ret = efi_load_image(data->os_file, &loaded_image, &handle);
	if (ret)
		return ret;

	image_header = (struct x86_setup_header *)loaded_image->image_base;

	if (!is_x86_setup_header(image_header) ||
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
		boot_header->ramdisk_image = efi_virt_to_phys(initrd);
		boot_header->ramdisk_size = PAGE_ALIGN(size);
	}

	options = linux_bootargs_get();
	if (options) {
		boot_header->cmd_line_ptr = efi_virt_to_phys(options);
		boot_header->cmdline_size = strlen(options);
	}

	boot_header->code32_start = efi_virt_to_phys(loaded_image->image_base +
			(image_header->setup_sects+1) * 512);

	printf("Booting kernel via handover");
	if (bootm_verbose(data)) {
		printf("at 0x%p", loaded_image->image_base);
		if (data->initrd_file)
			printf(", initrd at 0x%08x",
			       boot_header->ramdisk_image);
	}
	printf("...\n");

	if (data->dryrun) {
		BS->unload_image(handle);
		free(boot_header);
		free(initrd);
		return 0;
	}

	efi_set_variable_usec("LoaderTimeExecUSec", &efi_systemd_vendor_guid,
			      ktime_to_us(ktime_get()));

	shutdown_barebox();
	linux_efi_handover(handle, boot_header);

	return 0;
}

struct image_handler efi_x86_linux_handle_handover = {
	.name = "EFI X86 Linux kernel (Legacy Handover)",
	.bootm = do_bootm_efi,
	.filetype = filetype_x86_efi_linux_image,
	.check_image = efi_x86_boot_method_check,
};

static int efi_register_handover_handler(void)
{
	register_image_handler(&efi_x86_linux_handle_handover);
	return 0;
}
late_efi_initcall(efi_register_handover_handler);

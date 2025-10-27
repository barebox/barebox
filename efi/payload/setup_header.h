/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_PAYLOAD_SETUP_HEADER_H__
#define __EFI_PAYLOAD_SETUP_HEADER_H__

#include <linux/types.h>

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

#endif

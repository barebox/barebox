/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
 *
 * In parts from the GRUB2 project:
 *
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008  Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <fs.h>
#include <errno.h>
#include <libfile.h>
#include <getopt.h>
#include <malloc.h>
#include <boot.h>
#include <asm/syslib.h>

/** FIXME */
#define LINUX_MAGIC_SIGNATURE       0x53726448      /* "HdrS" */

/** FIXME */
#define LINUX_FLAG_BIG_KERNEL       0x1

/** FIXME */
#define LINUX_BOOT_LOADER_TYPE      0x72

/** FIXME */
#define LINUX_DEFAULT_SETUP_SECTS	4

/** FIXME */
#define LINUX_MAX_SETUP_SECTS		64

/** FIXME */
#define LINUX_OLD_REAL_MODE_SEGMT	0x9000

/** FIXME */
#define LINUX_OLD_REAL_MODE_ADDR	(LINUX_OLD_REAL_MODE_SEGMT << 4)

/** FIXME */
#define LINUX_HEAP_END_OFFSET		(LINUX_OLD_REAL_MODE_SEGMT - 0x200)

/** FIXME */
#define LINUX_FLAG_CAN_USE_HEAP		0x80

/** Define kernel command lines's start offset in the setup segment */
#define LINUX_CL_OFFSET			0x9000

/** Define kernel command lines's end offset */
#define LINUX_CL_END_OFFSET		0x90FF

/** FIXME */
#define LINUX_CL_MAGIC			0xA33F

/** FIXME */
#define LINUX_SETUP_MOVE_SIZE		0x9100

/** Sector size */
#define DISK_SECTOR_BITS		9
#define DISK_SECTOR_SIZE		0x200

/** Where to load a bzImage */
#define LINUX_BZIMAGE_ADDR		0x100000

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
#define LINUX_LOADER_ID_LILO		0x0
#define LINUX_LOADER_ID_LOADLIN		0x1
#define LINUX_LOADER_ID_BOOTSECT	0x2
#define LINUX_LOADER_ID_SYSLINUX	0x3
#define LINUX_LOADER_ID_ETHERBOOT	0x4
#define LINUX_LOADER_ID_ELILO		0x5
#define LINUX_LOADER_ID_GRUB		0x7
#define LINUX_LOADER_ID_UBOOT		0x8
#define LINUX_LOADER_ID_XEN		0x9
#define LINUX_LOADER_ID_GUJIN		0xa
#define LINUX_LOADER_ID_QEMU		0xb
	uint8_t loadflags;		/**< Boot protocol option flags */
	uint16_t setup_move_size;	/**< Move to high memory size */
	uint32_t code32_start;		/**< Boot loader hook */
	uint32_t ramdisk_image;		/**< initrd load address */
	uint32_t ramdisk_size;		/**< initrd size */
	uint32_t bootsect_kludge;	/**< obsolete */
	uint16_t heap_end_ptr;		/**< Free memory after setup end */
	uint8_t ext_loader_ver;		/**< boot loader's extension of the version number */
	uint8_t ext_loader_type;	/**< boot loader's extension of its type */
	char *cmd_line_ptr;		/**< Points to the kernel command line */
	uint32_t initrd_addr_max;	/**< Highest address for initrd */
#if 0
	/* for the records only. These members are defined in
	 * more recent Linux kernels
	 */
	uint32_t kernel_alignment;	/**< Alignment unit required by the kernel */
	uint8_t relocatable_kernel;	/** */
	uint8_t min_alignment;		/** */
	uint32_t cmdline_size;		/** */
	uint32_t hardware_subarch;	/** */
	uint64_t hardware_subarch_data;	/** */
	uint32_t payload_offset;	/** */
	uint32_t payload_length;	/** */
	uint64_t setup_data;		/** */
	uint64_t pref_address;		/** */
	uint32_t init_size;		/** */
#endif
} __attribute__ ((packed));

/* This is -1. Keep this value in sync with the kernel */
#define NORMAL_VGA	0xffff		/* 80x25 mode */
#define ASK_VGA		0xfffd		/* ask for it at bootup */

/**
 * Load an x86 Linux kernel bzImage and start it
 * @param argc parameter count
 * @param argv list of parameter
 *
 * Loads an x86 bzImage, checks for its integrity, stores the two parts
 * (setup = 'real mode code' and kernel = 'protected mode code') to their
 * default locations, switches back to real mode and runs the setup code.
 */
static int do_linux16(int argc, char *argv[])
{
	struct linux_kernel_header *lh = NULL;
	int rc, opt;
	unsigned setup_sects;
	unsigned real_mode_size;
	int vid_mode = NORMAL_VGA;
	size_t image_size;
	const char *cmdline = linux_bootargs_get();
	const char *kernel_file;

	while((opt = getopt(argc, argv, "v:")) > 0) {
		switch(opt) {
		case 'v':
			vid_mode = simple_strtoul(optarg, NULL, 0);
			if (vid_mode == 0) {
				if (!strcmp(optarg, "ask"))
					vid_mode = ASK_VGA;
				else {
					printf("Unknown video mode: %s\n", optarg);
					return 1;
				}
			}
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc) {
		printf("No kernel filename given\n");
		return 1;
	}
	kernel_file = argv[optind];

	lh = read_file(kernel_file, &image_size);
	if (lh == NULL) {
		printf("Cannot read file '%s'\n", argv[1]);
		return 1;
	}

	if (lh->boot_flag != 0xaa55) {
		printf("File '%s' has invalid magic number\n", argv[1]);
		rc = 1;
		goto on_error;
	}

	if (lh->setup_sects > LINUX_MAX_SETUP_SECTS) {
		printf("File '%s' contains too many setup sectors\n", argv[1]);
		rc = 1;
		goto on_error;
	}

	setup_sects = lh->setup_sects;

	printf("Found a %d.%d image header\n", lh->version >> 8, lh->version & 0xFF);

	if (lh->header == LINUX_MAGIC_SIGNATURE && lh->version >= 0x0200) {
		/* kernel is recent enough */
		;
		if (!(lh->loadflags & LINUX_FLAG_BIG_KERNEL)) {
			printf("Cannot load a classic zImage. Use a bzImage instead\n");
			goto on_error;
		}
		lh->type_of_loader = LINUX_BOOT_LOADER_TYPE;	/* TODO */

		if (lh->version >= 0x0201) {
			lh->heap_end_ptr = LINUX_HEAP_END_OFFSET;
			lh->loadflags |= LINUX_FLAG_CAN_USE_HEAP;
		}

		if (lh->version >= 0x0202)
			lh->cmd_line_ptr = (void*)(LINUX_OLD_REAL_MODE_ADDR + LINUX_CL_OFFSET);	/* FIXME */
		else {
			lh->cl_magic = LINUX_CL_MAGIC;
			lh->cl_offset = LINUX_CL_OFFSET;
			lh->setup_move_size = LINUX_SETUP_MOVE_SIZE;
		}
	} else {
		printf("Kernel too old to handle\n");
		rc = 1;
		goto on_error;
	}

	if (strlen(cmdline) >= (LINUX_CL_END_OFFSET -  LINUX_CL_OFFSET)) {
		printf("Kernel command line exceeds the available space\n");
		rc = 1;
		goto on_error;
	}

	/*
	 * The kernel does not check for the "vga=<val>" kernel command line
	 * parameter anymore. It expects this kind of information in the
	 * boot parameters instead.
	 */
	if (vid_mode != NORMAL_VGA)
		lh->vid_mode = vid_mode;

	/* If SETUP_SECTS is not set, set it to the default.  */
	if (setup_sects == 0) {
		printf("Fixing setup sector count\n");
		setup_sects = LINUX_DEFAULT_SETUP_SECTS;
	}

	if (setup_sects >= 15) {
		void *src = lh;
		if (lh->kernel_version != 0)
			printf("Kernel version: '%s'\n",
			       (char *)src + lh->kernel_version + DISK_SECTOR_SIZE);
	}

	/*
	 * Size of the real mode part to handle in a separate way
	 */
	real_mode_size = (setup_sects << DISK_SECTOR_BITS) + DISK_SECTOR_SIZE;

	/*
	 *                real mode space                     hole            extended memory
	 * |---------------------------------------------->|----------->|------------------------------>
	 * 0                                            0xa0000     0x100000
	 *  <-1-|----------2-----------><-3-   |
	 *    0x7e00                        0x90000
	 *                                <-4--|-5-->                   |---------6------------->
	 *
	 * 1) real mode stack
	 * 2) barebox code
	 * 3) flat mode stack
	 * 4) realmode stack when starting a Linux kernel
	 * 5) Kernel's real mode setup code
	 * 6) compressed kernel image
	 */
	/*
	 * Parts of the image we know:
	 * - real mode part
	 * - kernel payload
	 */
	/*
	 * NOTE: This part is dangerous, as it copies some image content to
	 * various locations in the main memory. This could overwrite important
	 * data of the running barebox (hopefully not)
	 */
	/* copy the real mode part of the image to the 9th segment */
	memcpy((void*)LINUX_OLD_REAL_MODE_ADDR, lh, LINUX_SETUP_MOVE_SIZE);

	/* TODO add 'BOOT_IMAGE=<file>' and 'auto' if no user intervention was done (in front of all other params) */
	/* copy also the command line into this area */
	memcpy((void*)(LINUX_OLD_REAL_MODE_ADDR + LINUX_CL_OFFSET), cmdline, strlen(cmdline) + 1);
	printf("Using kernel command line: '%s'\n", cmdline);

	/* copy the compressed image part to its final address the setup code expects it
	 * Note: The protected mode part starts at offset (setup_sects + 1) * 512
	 */
	memcpy((void*)LINUX_BZIMAGE_ADDR, ((void*)lh) + real_mode_size, image_size - real_mode_size);

	/*
	 * switch back to real mode now and start the real mode part of the
	 * image at address "(LINUX_OLD_REAL_MODE_ADDR >> 4) + 0x20:0x0000"
	 * which means "0x9020:0x000" -> 0x90200
	 */
	bios_start_linux(LINUX_OLD_REAL_MODE_SEGMT);	/* does not return */

on_error:
	if (lh != NULL)
		free(lh);

	return rc;
}

BAREBOX_CMD_HELP_START(linux16)
BAREBOX_CMD_HELP_TEXT("Load kernel from FILE and boot on x86 in real-mode.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Only kernel images in bzImage format are supported by now.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("For the video mode refer the Linux kernel documentation")
BAREBOX_CMD_HELP_TEXT("'Documentation/fb/vesafb.txt' for correct VESA mode numbers. Use 'ask'")
BAREBOX_CMD_HELP_TEXT("instead of a number to make Linux for options..")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-v VESAMODE",   "set VESAMODE")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(linux16)
	.cmd		= do_linux16,
	BAREBOX_CMD_DESC("boot a linux kernel on x86 via real-mode code")
	BAREBOX_CMD_OPTS("[-v VESAMODE] FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_linux16_help)
BAREBOX_CMD_END

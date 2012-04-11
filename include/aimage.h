/*
 * (C) Copyright 2011 Jean-Christhophe PLAGNIOL-VILLARD <plagniol@jcrosoft.com>
 *
 * Android boot image
 *
 * Under GPLv2 only
 */

#ifndef __AIMAGE_H__
#define __AIMAGE_H__

#define BOOT_MAGIC	"ANDROID!"
#define BOOT_MAGIC_SIZE	8
#define BOOT_NAME_SIZE	16
#define BOOT_ARGS_SIZE	512

struct android_header_comp
{
	unsigned size;
	unsigned load_addr;
};

struct android_header
{
	u8 magic[BOOT_MAGIC_SIZE];

	struct android_header_comp kernel;

	struct android_header_comp ramdisk;

	struct android_header_comp second_stage;

	/* physical addr for kernel tags */
	unsigned tags_addr;
	/* flash page size we assume */
	unsigned page_size;
	/* future expansion: should be 0 */
	unsigned unused[2];

	unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

	unsigned char cmdline[BOOT_ARGS_SIZE];

	unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

/*
 * +-----------------+
 * | boot header     | 1 page
 * +-----------------+
 * | kernel          | n pages
 * +-----------------+
 * | ramdisk         | m pages
 * +-----------------+
 * | second stage    | o pages
 * +-----------------+
 *
 * n = (kernel_size + page_size - 1) / page_size
 * m = (ramdisk_size + page_size - 1) / page_size
 * o = (second_size + page_size - 1) / page_size
 *
 * 0. all entities are page_size aligned in flash
 * 1. kernel and ramdisk are required (size != 0)
 * 2. second is optional (second_size == 0 -> no second)
 * 3. load each element (kernel, ramdisk, second) at
 *    the specified physical address (kernel_addr, etc)
 * 4. prepare tags at tag_addr.  kernel_args[] is
 *    appended to the kernel commandline in the tags.
 * 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
 * 6. if second_size != 0: jump to second_addr
 *    else: jump to kernel_addr
 */

#endif /* __AIMAGE_H__ */

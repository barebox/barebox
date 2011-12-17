#ifndef __BOOT_H
#define __BOOT_H

#include <image.h>
#include <filetype.h>
#include <of.h>
#include <linux/list.h>

struct image_data {
	/* simplest case. barebox has already loaded the os here */
	struct resource *os_res;

	/* if os is an uImage this will be provided */
	struct uimage_handle *os;
	int os_num;

	/* otherwise only the filename will be provided */
	char *os_file;

	/*
	 * The address the user wants to load the os image to.
	 * May be UIMAGE_INVALID_ADDRESS to indicate that the
	 * user has not specified any address. In this case the
	 * handler may choose a suitable address
	 */
	unsigned long os_address;

	/* entry point to the os. relative to the start of the image */
	unsigned long os_entry;

	/* if initrd is already loaded this resource will be !NULL */
	struct resource *initrd_res;

	/* if initrd is an uImage this will be provided */
	struct uimage_handle *initrd;
	int initrd_num;

	/* otherwise only the filename will be provided */
	char *initrd_file;

	unsigned long initrd_address;

	struct fdt_header *oftree;

	int verify;
	int verbose;
};

struct image_handler {
	const char *name;

	struct list_head list;

	int ih_os;

	enum filetype filetype;
	int (*bootm)(struct image_data *data);
};

int register_image_handler(struct image_handler *handle);

#ifdef CONFIG_CMD_BOOTM_VERBOSE
static inline int bootm_verbose(struct image_data *data)
{
	return data->verbose;
}
#else
static inline int bootm_verbose(struct image_data *data)
{
	return 0;
}
#endif

#endif /* __BOOT_H */

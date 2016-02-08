#ifndef __BOOT_H
#define __BOOT_H

#include <image.h>
#include <filetype.h>
#include <of.h>
#include <linux/list.h>
#include <environment.h>

enum bootm_verify {
	BOOTM_VERIFY_NONE,
	BOOTM_VERIFY_HASH,
	BOOTM_VERIFY_SIGNATURE,
};

struct bootm_data {
	const char *os_file;
	const char *initrd_file;
	const char *oftree_file;
	int verbose;
	enum bootm_verify verify;
	bool force;
	bool dryrun;
	unsigned long initrd_address;
	unsigned long os_address;
	unsigned long os_entry;
};

int bootm_boot(struct bootm_data *data);

struct image_data {
	/* simplest case. barebox has already loaded the os here */
	struct resource *os_res;

	/* if os is an uImage this will be provided */
	struct uimage_handle *os;

	/* if os is a FIT image this will be provided */
	struct fit_handle *os_fit;

	char *os_part;

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
	char *initrd_part;

	/* otherwise only the filename will be provided */
	char *initrd_file;

	unsigned long initrd_address;

	char *oftree_file;
	char *oftree_part;

	struct device_node *of_root_node;
	struct fdt_header *oftree;
	struct resource *oftree_res;

	enum bootm_verify verify;
	int verbose;
	int force;
	int dryrun;
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

#ifdef CONFIG_FLEXIBLE_BOOTARGS
const char *linux_bootargs_get(void);
int linux_bootargs_overwrite(const char *bootargs);
#else
static inline const char *linux_bootargs_get(void)
{
	return getenv("bootargs");
}

static inline int linux_bootargs_overwrite(const char *bootargs)
{
	return setenv("bootargs", bootargs);
}
#endif

void bootm_data_init_defaults(struct bootm_data *data);

int bootm_load_os(struct image_data *data, unsigned long load_address);

bool bootm_has_initrd(struct image_data *data);
int bootm_load_initrd(struct image_data *data, unsigned long load_address);

int bootm_load_devicetree(struct image_data *data, unsigned long load_address);
int bootm_get_os_size(struct image_data *data);

enum bootm_verify bootm_get_verify_mode(void);

#define UIMAGE_SOME_ADDRESS (UIMAGE_INVALID_ADDRESS - 1)

#endif /* __BOOT_H */

#ifndef __BOOT_H
#define __BOOT_H

#include <image.h>
#include <linux/list.h>

struct image_data {
	struct image_handle *os;
	struct image_handle *initrd;
	const char *oftree;
	int verify;
};

struct image_handler {
	struct list_head list;

	char *cmdline_options;
	int (*cmdline_parse)(struct image_data *data, int opt, char *optarg);
	char *help_string;

	int image_type;
	int (*bootm)(struct image_data *data);
};

int register_image_handler(struct image_handler *handle);

#endif /* __BOOT_H */


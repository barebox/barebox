/*
 * am33xx_bbu_nand_xloadslots.c - barebox update handler for
 * the nand xload slots.
 *
 * Copyright (c) 2014 Wadim Egorov <w.egorov@phytec.de>, Phytec
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <bbu.h>
#include <fs.h>
#include <fcntl.h>
#include <libfile.h>
#include <filetype.h>
#include <mach/bbu.h>

struct nand_bbu_handler {
	struct bbu_handler bbu_handler;
	char **devicefile;
	int num_devicefiles;
};

/*
 * Upate given nand partitions with an image
 */
static int nand_slot_update_handler(struct bbu_handler *handler,
						struct bbu_data *data)
{
	int ret, i;
	struct nand_bbu_handler *nh;
	const void *image = data->image;
	size_t size = data->len;

	nh = container_of(handler, struct nand_bbu_handler, bbu_handler);

	ret = bbu_confirm(data);
	if (ret != 0)
		return ret;

	/* check if the devicefile has been overwritten */
	if (strcmp(data->devicefile, nh->devicefile[0]) != 0) {
		ret = write_file_flash(data->devicefile, image, size);
		if (ret != 0)
			return ret;
	} else {
		for (i = 0; i < nh->num_devicefiles; i++) {
			ret =  write_file_flash(nh->devicefile[i], image, size);
			if (ret != 0)
				return ret;
		}
	}

	return 0;
}

/*
 * This handler updates all given xload slots in nand with an image.
 */
static int nand_xloadslots_update_handler(struct bbu_handler *handler,
					struct bbu_data *data)
{
	const void *image = data->image;
	size_t size = data->len;

	if (file_detect_type(image, size) != filetype_ch_image) {
		pr_err("%s is not a valid ch-image\n", data->imagefile);
		return -EINVAL;
	}

	return nand_slot_update_handler(handler, data);
}

int am33xx_bbu_nand_xloadslots_register_handler(const char *name,
						char **devicefile,
						int num_devicefiles)
{
	struct nand_bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->num_devicefiles = num_devicefiles;
	handler->bbu_handler.devicefile = devicefile[0];
	handler->bbu_handler.handler = nand_xloadslots_update_handler;
	handler->bbu_handler.name = name;

	ret = bbu_register_handler(&handler->bbu_handler);
	if (ret)
		free(handler);

	return ret;
}

static int nand_update_handler(struct bbu_handler *handler,
		struct bbu_data *data)
{
	const void *image = data->image;
	size_t size = data->len;

	if (file_detect_type(image, size) != filetype_arm_barebox) {
		pr_err("%s is not a valid barebox image\n", data->imagefile);
		return -EINVAL;
	}

	return nand_slot_update_handler(handler, data);
}

int am33xx_bbu_nand_slots_register_handler(const char *name, char **devicefile,
							int num_devicefiles)
{
	struct nand_bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->num_devicefiles = num_devicefiles;
	handler->bbu_handler.devicefile = devicefile[0];
	handler->bbu_handler.handler = nand_update_handler;
	handler->bbu_handler.name = name;

	ret = bbu_register_handler(&handler->bbu_handler);
	if (ret)
		free(handler);

	return ret;
}

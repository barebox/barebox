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
#include <filetype.h>

struct nand_bbu_handler {
	struct bbu_handler bbu_handler;
	char **devicefile;
	int num_devicefiles;
};

static int write_image(const char *devfile, const void *image, size_t size)
{
	int fd = 0;
	int ret = 0;

	fd = open(devfile, O_WRONLY);
	if (fd < 0) {
		pr_err("could not open %s: %s\n", devfile,
			errno_str());
		return fd;
	}

	ret = erase(fd, ~0, 0);
	if (ret < 0) {
		pr_err("could not erase %s: %s\n", devfile,
			errno_str());
		close(fd);
		return ret;
	}

	ret = write(fd, image, size);
	if (ret < 0) {
		pr_err("could not write to fd %s: %s\n", devfile,
			errno_str());
		close(fd);
		return ret;
	}

	close(fd);

	return 0;
}

/*
 * This handler updates all given xload slots in nand with an image.
 */
static int nand_xloadslots_update_handler(struct bbu_handler *handler,
					struct bbu_data *data)
{
	int ret = 0;
	const void *image = data->image;
	size_t size = data->len;
	struct nand_bbu_handler *nh;
	int i = 0;

	if (file_detect_type(image, size) != filetype_ch_image) {
		pr_err("%s is not a valid ch-image\n", data->imagefile);
		return -EINVAL;
	}

	nh = container_of(handler, struct nand_bbu_handler, bbu_handler);

	/* check if the devicefile has been overwritten */
	if (strcmp(data->devicefile, nh->devicefile[0]) != 0) {
		ret = bbu_confirm(data);
		if (ret != 0)
			return ret;

		ret = write_image(data->devicefile, image, size);
		if (ret != 0)
			return ret;
	} else {
		for (i = 0; i < nh->num_devicefiles; i++) {
			ret =  write_image(nh->devicefile[i], image, size);
			if (ret != 0)
				return ret;
		}
	}

	return 0;
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

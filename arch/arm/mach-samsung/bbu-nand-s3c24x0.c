/*
 * Copyright (C) 2014 Michael Olbrich, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */

#include <common.h>
#include <malloc.h>
#include <bbu.h>
#include <fs.h>
#include <fcntl.h>

static int nand_update(struct bbu_handler *handler, struct bbu_data *data)
{
	int fd, ret;

	if (file_detect_type(data->image, data->len) != filetype_arm_barebox &&
			!bbu_force(data, "Not an ARM barebox image"))
		return -EINVAL;

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	fd = open(data->devicefile, O_WRONLY);
	if (fd < 0)
		return fd;

	debug("%s: eraseing %s from 0 to 0x%08x\n", __func__,
			data->devicefile, data->len);
	ret = erase(fd, data->len, 0);
	if (ret) {
		printf("erasing %s failed with %s\n", data->devicefile,
				strerror(-ret));
		goto err_close;
	}

	ret = write(fd, data->image, data->len);
	if (ret < 0) {
		printf("writing update to %s failed with %s\n", data->devicefile,
				strerror(-ret));
		goto err_close;
	}

	ret = 0;

err_close:
	close(fd);

	return ret;
}

/*
 * Register a s3c24x0 update handler for NAND
 */
int s3c24x0_bbu_nand_register_handler(void)
{
	struct bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = "/dev/nand0.barebox";
	handler->name = "nand";
	handler->handler = nand_update;
	handler->flags = BBU_HANDLER_FLAG_DEFAULT;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}

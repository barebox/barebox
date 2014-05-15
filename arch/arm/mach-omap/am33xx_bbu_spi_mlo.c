/*
 * am33xx_bbu_spi_mlo.c - am35xx and am33xx specific MLO
 *	update handler for SPI NOR flash
 *
 * Copyright (c) 2013 Sharavn kumar <shravan.k@phytec.in>, Phytec
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
#include <bbu.h>
#include <fs.h>
#include <fcntl.h>

/*
 * AM35xx, AM33xx chips use big endian MLO for SPI NOR flash
 * This handler converting MLO to big endian and write to SPI NOR
 */
static int spi_nor_mlo_handler(struct bbu_handler *handler,
					struct bbu_data *data)
{
	int dstfd = 0;
	int ret = 0;
	uint32_t readbuf;
	int size = data->len;
	void *image = data->image;
	uint32_t *header;
	int swap = 0;

	header = data->image;
	if (header[5] == 0x43485345) {
		swap = 0;
	} else if (header[5] == 0x45534843) {
		swap = 1;
	} else {
		if (!bbu_force(data, "Not a MLO image"))
			return -EINVAL;
	}

	dstfd = open(data->devicefile, O_WRONLY);
	if (dstfd < 0) {
		printf("could not open %s: %s", data->devicefile, errno_str());
		ret = dstfd;
		goto out;
	}

	ret = erase(dstfd, ~0, 0);
	if (ret < 0) {
		printf("could not erase %s: %s", data->devicefile, errno_str());
		goto out1;
	}

	for (; size >= 0; size -= 4) {
		memcpy((char *)&readbuf, image, 4);

		if (swap)
			readbuf = cpu_to_be32(readbuf);
		ret = write(dstfd, &readbuf, 4);
		if (ret < 0) {
			perror("write");
			goto out1;
		}

		image = image + 4;
	}

	ret = 0;
out1:
	close(dstfd);
out:
	return ret;
}

static int spi_nor_handler(struct bbu_handler *handler,
					struct bbu_data *data)
{
	int fd, ret;

	if (file_detect_type(data->image, data->len) != filetype_arm_barebox) {
		if (!bbu_force(data, "Not an ARM barebox image"))
			return -EINVAL;
	}

	fd = open(data->devicefile, O_RDWR | O_CREAT);
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
	if (ret < 0)
		goto err_close;

	ret = 0;

err_close:
	close(fd);

	return ret;
}

/*
 * Register a am33xx MLO update handler for SPI NOR
 */
int am33xx_bbu_spi_nor_mlo_register_handler(const char *name, char *devicefile)
{
	struct bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->name = name;
	handler->handler = spi_nor_mlo_handler;

	ret = bbu_register_handler(handler);

	if (ret)
		free(handler);

	return ret;
}

/*
 * Register a am33xx update handler for SPI NOR
 */
int am33xx_bbu_spi_nor_register_handler(const char *name, char *devicefile)
{
	struct bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->name = name;
	handler->handler = spi_nor_handler;

	ret = bbu_register_handler(handler);

	if (ret)
		free(handler);

	return ret;
}

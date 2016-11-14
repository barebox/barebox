/*
 * Copyright (C) 2016 Pengutronix, Markus Pargmann <mpa@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <fcntl.h>
#include <fs.h>
#include <libfile.h>
#include <linux/kernel.h>
#include <malloc.h>
#include <printk.h>

#include "state.h"

struct state_backend_storage_bucket_direct {
	struct state_backend_storage_bucket bucket;

	ssize_t offset;
	ssize_t max_size;

	int fd;

	struct device_d *dev;
};

struct __attribute__((__packed__)) state_backend_storage_bucket_direct_meta {
	uint32_t magic;
	uint32_t written_length;
};
static const uint32_t direct_magic = 0x2354fdf3;

static inline struct state_backend_storage_bucket_direct
    *get_bucket_direct(struct state_backend_storage_bucket *bucket)
{
	return container_of(bucket, struct state_backend_storage_bucket_direct,
			    bucket);
}

static int state_backend_bucket_direct_read(struct state_backend_storage_bucket
					    *bucket, uint8_t ** buf_out,
					    ssize_t * len_hint)
{
	struct state_backend_storage_bucket_direct *direct =
	    get_bucket_direct(bucket);
	struct state_backend_storage_bucket_direct_meta meta;
	ssize_t read_len;
	uint8_t *buf;
	int ret;

	ret = lseek(direct->fd, direct->offset, SEEK_SET);
	if (ret < 0) {
		dev_err(direct->dev, "Failed to seek file, %d\n", ret);
		return ret;
	}
	ret = read_full(direct->fd, &meta, sizeof(meta));
	if (ret < 0) {
		dev_err(direct->dev, "Failed to read meta data from file, %d\n", ret);
		return ret;
	}
	if (meta.magic == direct_magic) {
		read_len = meta.written_length;
	} else {
		if (*len_hint)
			read_len = *len_hint;
		else
			read_len = direct->max_size;
		ret = lseek(direct->fd, direct->offset, SEEK_SET);
		if (ret < 0) {
			dev_err(direct->dev, "Failed to seek file, %d\n", ret);
			return ret;
		}
	}
	if (direct->max_size)
		read_len = min(read_len, direct->max_size);

	buf = xmalloc(read_len);
	if (!buf)
		return -ENOMEM;

	ret = read_full(direct->fd, buf, read_len);
	if (ret < 0) {
		dev_err(direct->dev, "Failed to read from file, %d\n", ret);
		free(buf);
		return ret;
	}

	*buf_out = buf;
	*len_hint = read_len;

	return 0;
}

static int state_backend_bucket_direct_write(struct state_backend_storage_bucket
					     *bucket, const uint8_t * buf,
					     ssize_t len)
{
	struct state_backend_storage_bucket_direct *direct =
	    get_bucket_direct(bucket);
	int ret;
	struct state_backend_storage_bucket_direct_meta meta;

	if (direct->max_size && len > direct->max_size)
		return -E2BIG;

	ret = lseek(direct->fd, direct->offset, SEEK_SET);
	if (ret < 0) {
		dev_err(direct->dev, "Failed to seek file, %d\n", ret);
		return ret;
	}

	meta.magic = direct_magic;
	meta.written_length = len;
	ret = write_full(direct->fd, &meta, sizeof(meta));
	if (ret < 0) {
		dev_err(direct->dev, "Failed to write metadata to file, %d\n", ret);
		return ret;
	}

	ret = write_full(direct->fd, buf, len);
	if (ret < 0) {
		dev_err(direct->dev, "Failed to write file, %d\n", ret);
		return ret;
	}

	ret = flush(direct->fd);
	if (ret < 0) {
		dev_err(direct->dev, "Failed to flush file, %d\n", ret);
		return ret;
	}

	return 0;
}

static void state_backend_bucket_direct_free(struct
					     state_backend_storage_bucket
					     *bucket)
{
	struct state_backend_storage_bucket_direct *direct =
	    get_bucket_direct(bucket);

	close(direct->fd);
	free(direct);
}

int state_backend_bucket_direct_create(struct device_d *dev, const char *path,
				       struct state_backend_storage_bucket **bucket,
				       off_t offset, ssize_t max_size)
{
	int fd;
	struct state_backend_storage_bucket_direct *direct;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		dev_err(dev, "Failed to open file '%s', %d\n", path, -errno);
		close(fd);
		return -errno;
	}

	direct = xzalloc(sizeof(*direct));
	direct->offset = offset;
	direct->max_size = max_size;
	direct->fd = fd;
	direct->dev = dev;

	direct->bucket.read = state_backend_bucket_direct_read;
	direct->bucket.write = state_backend_bucket_direct_write;
	direct->bucket.free = state_backend_bucket_direct_free;
	*bucket = &direct->bucket;

	return 0;
}

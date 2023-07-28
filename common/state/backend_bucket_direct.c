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
#include <linux/printk.h>

#include "state.h"

struct state_backend_storage_bucket_direct {
	struct state_backend_storage_bucket bucket;

	ssize_t offset;
	ssize_t max_size;

	int fd;

	struct device *dev;
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
					    *bucket, void ** buf_out,
					    ssize_t * len_out)
{
	struct state_backend_storage_bucket_direct *direct =
	    get_bucket_direct(bucket);
	struct state_backend_storage_bucket_direct_meta meta;
	uint32_t read_len, header_len = 0;
	void *buf;
	int ret;

	if (lseek(direct->fd, direct->offset, SEEK_SET) != direct->offset) {
		dev_err(direct->dev, "Failed to seek file, %d\n", -errno);
		return -errno;
	}
	ret = read_full(direct->fd, &meta, sizeof(meta));
	if (ret < 0) {
		dev_err(direct->dev, "Failed to read meta data from file, %d\n", ret);
		return ret;
	}
	if (meta.magic == direct_magic) {
		read_len = meta.written_length;
		if (read_len > direct->max_size) {
			dev_err(direct->dev, "Wrong length in meta data\n");
			return -EINVAL;

		}

		header_len = sizeof(meta);
	} else {
		if (meta.magic != ~0 && !!meta.magic)
			bucket->wrong_magic = 1;
		if (!IS_ENABLED(CONFIG_STATE_BACKWARD_COMPATIBLE)) {
			dev_dbg(direct->dev, "Enable backward compatibility or increase stride size\n");
			return dev_err_state_init(direct->dev, meta.magic ? -EINVAL : -ENOMEDIUM,
						  "No meta data header found\n");
		}
		read_len = direct->max_size;
		if (lseek(direct->fd, direct->offset, SEEK_SET) !=
		    direct->offset) {
			dev_err(direct->dev, "Failed to seek file, %d\n",
				-errno);
			return -errno;
		}

	}

	buf = xmalloc(read_len);
	if (!buf)
		return -ENOMEM;

	dev_dbg(direct->dev, "Read state from %lld length %d\n", (long long) direct->offset,
		header_len + read_len);

	ret = read_full(direct->fd, buf, read_len);
	if (ret < 0) {
		dev_err(direct->dev, "Failed to read from file, %d\n", ret);
		free(buf);
		return ret;
	}

	*buf_out = buf;
	*len_out = read_len;

	return 0;
}

static int state_backend_bucket_direct_write(struct state_backend_storage_bucket
					     *bucket, const void * buf,
					     ssize_t len)
{
	struct state_backend_storage_bucket_direct *direct =
	    get_bucket_direct(bucket);
	size_t header_len = 0;
	int ret;
	struct state_backend_storage_bucket_direct_meta meta;

	if (lseek(direct->fd, direct->offset, SEEK_SET) != direct->offset) {
		dev_err(direct->dev, "Failed to seek file, %d\n", -errno);
		return -errno;
	}

	/* write the meta data only if there is head room */
	if (len <= direct->max_size - sizeof(meta)) {
		meta.magic = direct_magic;
		meta.written_length = len;
		ret = write_full(direct->fd, &meta, sizeof(meta));
		if (ret < 0) {
			dev_err(direct->dev, "Failed to write metadata to file, %d\n", ret);
			return ret;
		}

		header_len = sizeof(meta);
	} else {
		if (!IS_ENABLED(CONFIG_STATE_BACKWARD_COMPATIBLE)) {
			dev_dbg(direct->dev, "Too small stride size: must skip metadata! Increase stride size\n");
			return -EINVAL;
		}
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

	dev_dbg(direct->dev, "Written state to offset %lld length %zu data length %zu\n",
		(long long)direct->offset, len + header_len, len);

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

int state_backend_bucket_direct_create(struct device *dev, const char *path,
				       struct state_backend_storage_bucket **bucket,
				       off_t offset, ssize_t max_size, bool readonly)
{
	int fd;
	struct state_backend_storage_bucket_direct *direct;

	fd = open(path, readonly ? O_RDONLY : O_RDWR);
	if (fd < 0) {
		dev_err(dev, "Failed to open file '%s', %d\n", path, -errno);
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

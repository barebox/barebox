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

#include <asm-generic/ioctl.h>
#include <fcntl.h>
#include <fs.h>
#include <libfile.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mtd/mtd-abi.h>
#include <linux/stat.h>
#include <malloc.h>
#include <printk.h>

#include "state.h"

/*
 * The state framework stores data in so called buckets. A bucket is
 * exactly one copy of the state we want to store. On flash type media
 * a bucket corresponds to a single eraseblock. On media which do not
 * need an erase operation a bucket corresponds to a storage area of
 * @stridesize bytes.
 *
 * For redundancy and to make sure that we have valid data on the storage
 * device at any time the state framework stores multiple buckets. The strategy
 * is as follows:
 *
 * When loading the state from the storage we iterate over the buckets. We
 * take the first one we find which has valid crcs. The next step is to
 * restore consistency between the different buckets. This means rewriting
 * a bucket when it signalled it needs refresh (i.e. returned -EUCLEAN)
 * or when contains data different from the bucket we use.
 *
 * When the state backend initialized successfully we already restored
 * consistency which means all buckets contain the same data. This means
 * when storing a new state we can just write all buckets in order.
 */

static const unsigned int min_buckets_written = 1;

/**
 * state_storage_write - Writes the given data to the storage
 * @param storage Storage object
 * @param buf Buffer with the data
 * @param len Length of the buffer
 * @return 0 on success, -errno otherwise
 *
 * This function iterates over all registered buckets and executes a write
 * operation on all of them. Writes are always in the same sequence. This
 * ensures, that reading in the same sequence will always return the latest
 * written valid data first.
 * We try to at least write min_buckets_written. If this fails we return with an
 * error.
 */
int state_storage_write(struct state_backend_storage *storage,
		        const void * buf, ssize_t len)
{
	struct state_backend_storage_bucket *bucket;
	int ret;
	int buckets_written = 0;

	if (storage->readonly)
		return 0;

	list_for_each_entry(bucket, &storage->buckets, bucket_list) {
		ret = bucket->write(bucket, buf, len);
		if (ret) {
			dev_warn(storage->dev, "Failed to write state backend bucket, %d\n",
				 ret);
		} else {
			++buckets_written;
		}
	}

	if (buckets_written >= min_buckets_written)
		return 0;

	dev_err(storage->dev, "Failed to write state to at least %d buckets. Successfully written to %d buckets\n",
		min_buckets_written, buckets_written);
	return -EIO;
}

static int bucket_refresh(struct state_backend_storage *storage,
			  struct state_backend_storage_bucket *bucket, void *buf, ssize_t len)
{
	int ret;

	if (bucket->needs_refresh)
		goto refresh;

	if (bucket->len != len)
		goto refresh;

	if (memcmp(bucket->buf, buf, len))
		goto refresh;

	return 0;

refresh:
	ret = bucket->write(bucket, buf, len);

	if (ret) {
		dev_warn(storage->dev, "Failed to restore bucket %d@0x%08lx\n",
			 bucket->num, bucket->offset);
	} else {
		dev_info(storage->dev, "restored bucket %d@0x%08lx\n",
			 bucket->num, bucket->offset);
		bucket->needs_refresh = 0;
	}

	return ret;
}

/**
 * state_storage_read - Reads valid data from the backend storage
 * @param storage Storage object
 * @param format Format of the data that is stored
 * @param magic state magic value
 * @param buf The newly allocated data area will be stored in this pointer
 * @param len The resulting length of the buffer
 * @param flags flags controlling how to load state
 * @return 0 on success, -errno otherwise. buf and len will be set to valid
 * values on success.
 *
 * This function goes through all buckets and tries to read valid data from
 * them. The first bucket which returns data that is successfully verified
 * against the data format is used. To ensure the validity of all bucket copies,
 * we restore the consistency at the end.
 */
int state_storage_read(struct state_backend_storage *storage,
		       struct state_backend_format *format,
		       uint32_t magic, void **buf, ssize_t *len,
		       enum state_flags flags)
{
	struct state_backend_storage_bucket *bucket, *bucket_used = NULL;
	int ret;

	/*
	 * Iterate over all buckets. The first valid one we find is the
	 * one we want to use.
	 */
	list_for_each_entry(bucket, &storage->buckets, bucket_list) {
		ret = bucket->read(bucket, &bucket->buf, &bucket->len);
		if (ret == -EUCLEAN)
			bucket->needs_refresh = 1;
		else if (ret)
			continue;

		/*
		 * Verify the buffer crcs. The buffer length is passed in the len argument,
		 * .verify overwrites it with the length actually used.
		 */
		ret = format->verify(format, magic, bucket->buf, &bucket->len, flags);
		if (!ret && !bucket_used)
			bucket_used = bucket;
	}

	if (!bucket_used) {
		dev_err(storage->dev, "Failed to find any valid state copy in any bucket\n");

		return -ENOENT;
	}

	dev_info(storage->dev, "Using bucket %d@0x%08lx\n", bucket_used->num, bucket_used->offset);

	/*
	 * Restore/refresh all buckets except the one we currently use (in case
	 * it's the only usable bucket at the moment)
	 */
	list_for_each_entry(bucket, &storage->buckets, bucket_list) {
		if (bucket == bucket_used)
			continue;

		ret = bucket_refresh(storage, bucket, bucket_used->buf, bucket_used->len);

		/* Free buffer from the unused buckets */
		free(bucket->buf);
		bucket->buf = NULL;
	}

	/*
	 * Restore/refresh the bucket we currently use
	 */
	ret = bucket_refresh(storage, bucket_used, bucket_used->buf, bucket_used->len);

	*buf = bucket_used->buf;
	*len = bucket_used->len;

	/* buffer from the used bucket is passed to the caller, do not free */
	bucket_used->buf = NULL;

	return 0;
}

static int mtd_get_meminfo(const char *path, struct mtd_info_user *meminfo)
{
	int fd, ret;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		pr_err("Failed to open '%s', %d\n", path, ret);
		return fd;
	}

	ret = ioctl(fd, MEMGETINFO, meminfo);

	close(fd);

	return ret;
}

/* Number of buckets that should be used */
static const int desired_buckets = 3;

/**
 * state_storage_mtd_buckets_init - Creates storage buckets for mtd devices
 * @param storage Storage object
 * @param meminfo Info about the mtd device
 * @param circular If false, use non-circular mode to write data that is compatible with the old on-flash format
 * @return 0 on success, -errno otherwise
 *
 * This function iterates over the eraseblocks and creates one bucket on
 * each eraseblock until we have the number of desired buckets. Bad blocks
 * will be skipped and the next block will be used.
 */
static int state_storage_mtd_buckets_init(struct state_backend_storage *storage,
					  struct mtd_info_user *meminfo, bool circular)
{
	struct state_backend_storage_bucket *bucket;
	ssize_t end = storage->offset + storage->max_size;
	int n_buckets = 0;
	off_t offset;
	ssize_t writesize;

	if (!end || end > meminfo->size)
		end = meminfo->size;

	if (!IS_ALIGNED(storage->offset, meminfo->erasesize)) {
		dev_err(storage->dev, "Offset within the device is not aligned to eraseblocks. Offset is %ld, erasesize %zu\n",
			storage->offset, meminfo->erasesize);
		return -EINVAL;
	}

	if (circular)
		writesize = meminfo->writesize;
	else
		writesize = meminfo->erasesize;

	for (offset = storage->offset; offset < end; offset += meminfo->erasesize) {
		int ret;
		unsigned int eraseblock = offset / meminfo->erasesize;

		ret = state_backend_bucket_circular_create(storage->dev, storage->path,
							   &bucket,
							   eraseblock,
							   writesize,
							   meminfo);
		if (ret)
			continue;

		bucket->offset = offset;
		bucket->num = n_buckets;

		list_add_tail(&bucket->bucket_list, &storage->buckets);
		++n_buckets;
		if (n_buckets >= desired_buckets)
			return 0;
	}

	if (!n_buckets) {
		dev_err(storage->dev, "Failed to initialize any state storage bucket\n");
		return -EIO;
	}

	dev_warn(storage->dev, "Failed to initialize desired amount of buckets, only %d of %d succeeded\n",
		 n_buckets, desired_buckets);
	return 0;
}

/**
 * state_storage_file_buckets_init - Create buckets for a conventional file descriptor
 * @param storage Storage object
 * @return 0 on success, -errno otherwise
 *
 * direct buckets are simpler than circular buckets and can be used on blockdevices
 * and mtd devices that don't need erase (MRAM). Also used for EEPROMs.
 */
static int state_storage_file_buckets_init(struct state_backend_storage *storage)
{
	struct state_backend_storage_bucket *bucket;
	int ret, n;
	off_t offset;
	int n_buckets = 0;
	uint32_t stridesize = storage->stridesize;
	size_t max_size = storage->max_size;

	if (!stridesize) {
		dev_err(storage->dev, "stridesize unspecified\n");
		return -EINVAL;
	}

	if (max_size && max_size < desired_buckets * stridesize) {
		dev_err(storage->dev, "device is too small to hold %d copies\n", desired_buckets);
		return -EINVAL;
	}

	for (n = 0; n < desired_buckets; n++) {
		offset = storage->offset + n * stridesize;
		ret = state_backend_bucket_direct_create(storage->dev, storage->path,
							 &bucket, offset,
							 stridesize);
		if (ret) {
			dev_warn(storage->dev, "Failed to create direct bucket at '%s' offset %ld\n",
				 storage->path, offset);
			continue;
		}

		bucket->offset = offset;
		bucket->num = n_buckets;

		list_add_tail(&bucket->bucket_list, &storage->buckets);
		++n_buckets;
	}

	if (!n_buckets) {
		dev_err(storage->dev, "Failed to initialize any state direct storage bucket\n");
		return -EIO;
	}

	if (n_buckets < desired_buckets)
		dev_warn(storage->dev, "Failed to initialize desired amount of direct buckets, only %d of %d succeeded\n",
			n_buckets, desired_buckets);

	return 0;
}


/**
 * state_storage_init - Init backend storage
 * @param path Path to the backend storage file
 * @param dev_offset Offset in the device to start writing at.
 * @param max_size Maximum size of the data. May be 0 for infinite.
 * @param stridesize Distance between two copies of the data. Not relevant for MTD
 * @param storagetype Type of the storage backend. This may be NULL where we
 * autoselect some backwardscompatible backend options
 * @return 0 on success, -errno otherwise
 *
 * Depending on the filetype, we create mtd buckets or normal file buckets.
 */
int state_storage_init(struct state *state, const char *path,
		       off_t offset, size_t max_size, uint32_t stridesize,
		       const char *storagetype)
{
	struct state_backend_storage *storage = &state->storage;
	int ret = -ENODEV;
	struct mtd_info_user meminfo;

	INIT_LIST_HEAD(&storage->buckets);
	storage->dev = &state->dev;
	storage->name = storagetype;
	storage->stridesize = stridesize;
	storage->offset = offset;
	storage->max_size = max_size;
	storage->path = xstrdup(path);

	if (IS_ENABLED(CONFIG_MTD))
		ret = mtd_get_meminfo(path, &meminfo);

	if (!ret && !(meminfo.flags & MTD_NO_ERASE)) {
		bool circular;
		if (!storagetype || !strcmp(storagetype, "circular")) {
			circular = true;
		} else if (!strcmp(storagetype, "noncircular")) {
			dev_warn(storage->dev, "using old format circular storage type.\n");
			circular = false;
		} else {
			dev_warn(storage->dev, "unknown storage type '%s'\n", storagetype);
			return -EINVAL;
		}
		return state_storage_mtd_buckets_init(storage, &meminfo, circular);
	} else {
		return state_storage_file_buckets_init(storage);
	}

	dev_err(storage->dev, "storage init done\n");
}

void state_storage_set_readonly(struct state_backend_storage *storage)
{
	storage->readonly = true;
}

/**
 * state_storage_free - Free backend storage
 * @param storage Storage object
 */
void state_storage_free(struct state_backend_storage *storage)
{
	struct state_backend_storage_bucket *bucket;
	struct state_backend_storage_bucket *bucket_tmp;

	if (!storage->buckets.next)
		return;

	list_for_each_entry_safe(bucket, bucket_tmp, &storage->buckets,
				 bucket_list) {
		list_del(&bucket->bucket_list);
		bucket->free(bucket);
	}

	free(storage->path);
}

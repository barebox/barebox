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

const unsigned int min_copies_written = 1;

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
 * We try to at least write min_copies_written. If this fails we return with an
 * error.
 */
int state_storage_write(struct state_backend_storage *storage,
		        const uint8_t * buf, ssize_t len)
{
	struct state_backend_storage_bucket *bucket;
	int ret;
	int copies_written = 0;

	if (storage->readonly)
		return 0;

	list_for_each_entry(bucket, &storage->buckets, bucket_list) {
		ret = bucket->write(bucket, buf, len);
		if (ret) {
			dev_warn(storage->dev, "Failed to write state backend bucket, %d\n",
				 ret);
		} else {
			++copies_written;
		}
	}

	if (copies_written >= min_copies_written)
		return 0;

	dev_err(storage->dev, "Failed to write state to at least %d buckets. Successfully written to %d buckets\n",
		min_copies_written, copies_written);
	return -EIO;
}

/**
 * state_storage_restore_consistency - Restore consistency on all storage backends
 * @param storage Storage object
 * @param buf Buffer with valid data that should be on all buckets after this operation
 * @param len Length of the buffer
 * @return 0 on success, -errno otherwise
 *
 * This function brings valid data onto all buckets we have to ensure that all
 * data copies are in sync. In the current implementation we just write the data
 * to all buckets. Bucket implementations that need to keep the number of writes
 * low, can read their own copy first and compare it.
 */
int state_storage_restore_consistency(struct state_backend_storage *storage,
				      const uint8_t * buf, ssize_t len)
{
	return state_storage_write(storage, buf, len);
}

/**
 * state_storage_read - Reads valid data from the backend storage
 * @param storage Storage object
 * @param format Format of the data that is stored
 * @param magic state magic value
 * @param buf The newly allocated data area will be stored in this pointer
 * @param len The resulting length of the buffer
 * @param len_hint Hint of how big the data may be.
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
		       uint32_t magic, uint8_t ** buf, ssize_t * len)
{
	struct state_backend_storage_bucket *bucket;
	int ret;

	list_for_each_entry(bucket, &storage->buckets, bucket_list) {
		*len = 0;

		ret = bucket->read(bucket, buf, len);
		if (ret) {
			dev_warn(storage->dev, "Failed to read from state backend bucket, trying next, %d\n",
				 ret);
			continue;
		}
		ret = format->verify(format, magic, *buf, *len);
		if (!ret) {
			goto found;
		}
		free(*buf);
		dev_warn(storage->dev, "Failed to verify read copy, trying next bucket, %d\n",
			 ret);
	}

	dev_err(storage->dev, "Failed to find any valid state copy in any bucket\n");

	return -ENOENT;

found:
	/* A failed restore consistency is not a failure of reading the state */
	state_storage_restore_consistency(storage, *buf, *len);

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

/* Number of copies that should be allocated */
const int desired_copies = 3;

/**
 * state_storage_mtd_buckets_init - Creates storage buckets for mtd devices
 * @param storage Storage object
 * @param meminfo Info about the mtd device
 * @param path Path to the device
 * @param circular If false, use non-circular mode to write data that is compatible with the old on-flash format
 * @param dev_offset Offset to start at in the device.
 * @param max_size Maximum size to use for data. May be 0 for infinite.
 * @return 0 on success, -errno otherwise
 *
 * Starting from offset 0 this function tries to create circular buckets on
 * different offsets in the device. Different copies of the data are located in
 * different eraseblocks.
 * For MTD devices we use circular buckets to minimize the number of erases.
 * Circular buckets write new data always in the next free space.
 */
static int state_storage_mtd_buckets_init(struct state_backend_storage *storage,
					  struct mtd_info_user *meminfo,
					  const char *path, bool circular,
					  off_t dev_offset, size_t max_size)
{
	struct state_backend_storage_bucket *bucket;
	ssize_t end = dev_offset + max_size;
	int nr_copies = 0;
	off_t offset;
	ssize_t writesize;

	if (!end || end > meminfo->size)
		end = meminfo->size;

	if (!IS_ALIGNED(dev_offset, meminfo->erasesize)) {
		dev_err(storage->dev, "Offset within the device is not aligned to eraseblocks. Offset is %ld, erasesize %zu\n",
			dev_offset, meminfo->erasesize);
		return -EINVAL;
	}

	if (circular)
		writesize = meminfo->writesize;
	else
		writesize = meminfo->erasesize;

	for (offset = dev_offset; offset < end; offset += meminfo->erasesize) {
		int ret;
		unsigned int eraseblock = offset / meminfo->erasesize;

		ret = state_backend_bucket_circular_create(storage->dev, path,
							   &bucket,
							   eraseblock,
							   writesize,
							   meminfo);
		if (ret) {
			dev_warn(storage->dev, "Failed to create bucket at '%s' eraseblock %u\n",
				 path, eraseblock);
			continue;
		}

		ret = state_backend_bucket_cached_create(storage->dev, bucket,
							 &bucket);
		if (ret) {
			dev_warn(storage->dev, "Failed to setup cache bucket, continuing without cache, %d\n",
				 ret);
		}

		list_add_tail(&bucket->bucket_list, &storage->buckets);
		++nr_copies;
		if (nr_copies >= desired_copies)
			return 0;
	}

	if (!nr_copies) {
		dev_err(storage->dev, "Failed to initialize any state storage bucket\n");
		return -EIO;
	}

	dev_warn(storage->dev, "Failed to initialize desired amount of buckets, only %d of %d succeeded\n",
		 nr_copies, desired_copies);
	return 0;
}

/**
 * state_storage_file_buckets_init - Create buckets for a conventional file descriptor
 * @param storage Storage object
 * @param path Path to file/device
 * @param dev_offset Offset in the device to start writing at.
 * @param max_size Maximum size of the data. May be 0 for infinite.
 * @param stridesize How far apart the different data copies are placed. If
 * stridesize is 0, only one copy can be created.
 * @return 0 on success, -errno otherwise
 *
 * For blockdevices and other regular files we create direct buckets beginning
 * at offset 0. Direct buckets are simple and write data always to offset 0.
 */
static int state_storage_file_buckets_init(struct state_backend_storage *storage,
					   const char *path, off_t dev_offset,
					   size_t max_size, uint32_t stridesize)
{
	struct state_backend_storage_bucket *bucket;
	int ret, n;
	off_t offset;
	int nr_copies = 0;

	if (!stridesize) {
		dev_err(storage->dev, "stridesize unspecified\n");
		return -EINVAL;
	}

	if (max_size && max_size < desired_copies * stridesize) {
		dev_err(storage->dev, "device is too small to hold %d copies\n", desired_copies);
		return -EINVAL;
	}

	for (n = 0; n < desired_copies; n++) {
		offset = dev_offset + n * stridesize;
		ret = state_backend_bucket_direct_create(storage->dev, path,
							 &bucket, offset,
							 stridesize);
		if (ret) {
			dev_warn(storage->dev, "Failed to create direct bucket at '%s' offset %ld\n",
				 path, offset);
			continue;
		}

		ret = state_backend_bucket_cached_create(storage->dev, bucket,
							 &bucket);
		if (ret) {
			dev_warn(storage->dev, "Failed to setup cache bucket, continuing without cache, %d\n",
				 ret);
		}

		list_add_tail(&bucket->bucket_list, &storage->buckets);
		++nr_copies;
	}

	if (!nr_copies) {
		dev_err(storage->dev, "Failed to initialize any state direct storage bucket\n");
		return -EIO;
	}

	if (nr_copies < desired_copies)
		dev_warn(storage->dev, "Failed to initialize desired amount of direct buckets, only %d of %d succeeded\n",
			nr_copies, desired_copies);

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

	if (IS_ENABLED(CONFIG_MTD))
		ret = mtd_get_meminfo(path, &meminfo);

	if (!ret && !(meminfo.flags & MTD_NO_ERASE)) {
		bool circular = true;
		if (!storagetype) {
			circular = false;
		} else if (strcmp(storagetype, "circular")) {
			dev_warn(storage->dev, "Unknown storagetype '%s', falling back to old format circular storage type.\n",
				 storagetype);
			circular = false;
		}
		return state_storage_mtd_buckets_init(storage, &meminfo, path,
						      circular, offset,
						      max_size);
	} else {
		return state_storage_file_buckets_init(storage, path, offset,
						       max_size, stridesize);
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
}

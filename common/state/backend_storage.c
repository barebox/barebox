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

static int bucket_lazy_init(struct state_backend_storage_bucket *bucket)
{
	int ret;

	if (bucket->initialized)
		return 0;

	if (bucket->init) {
		ret = bucket->init(bucket);
		if (ret)
			return ret;
	}
	bucket->initialized = true;

	return 0;
}

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
		ret = bucket_lazy_init(bucket);
		if (ret) {
			dev_warn(storage->dev, "Failed to init bucket/write state backend bucket, %d\n",
				 ret);
			continue;
		}

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
		       uint32_t magic, uint8_t ** buf, ssize_t * len,
		       ssize_t len_hint)
{
	struct state_backend_storage_bucket *bucket;
	int ret;

	list_for_each_entry(bucket, &storage->buckets, bucket_list) {
		*len = len_hint;
		ret = bucket_lazy_init(bucket);
		if (ret) {
			dev_warn(storage->dev, "Failed to init bucket/read state backend bucket, %d\n",
				 ret);
			continue;
		}

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

#ifdef __BAREBOX__
#define STAT_GIVES_SIZE(s) (S_ISREG(s.st_mode) || S_ISCHR(s.st_mode))
#define BLKGET_GIVES_SIZE(s) 0
#else
#define STAT_GIVES_SIZE(s) (S_ISREG(s.st_mode))
#define BLKGET_GIVES_SIZE(s) (S_ISBLK(s.st_mode))
#endif
#ifndef BLKGETSIZE64
#define BLKGETSIZE64 -1
#endif

static int state_backend_storage_get_size(const char *path, size_t * out_size)
{
	struct mtd_info_user meminfo;
	struct stat s;
	int ret;

	ret = stat(path, &s);
	if (ret)
		return -errno;

	/*
	 * under Linux, stat() gives the size only on regular files
	 * under barebox, it works on char dev, too
	 */
	if (STAT_GIVES_SIZE(s)) {
		*out_size = s.st_size;
		return 0;
	}

	/* this works under Linux on block devs */
	if (BLKGET_GIVES_SIZE(s)) {
		int fd;

		fd = open(path, O_RDONLY);
		if (fd < 0)
			return -errno;

		ret = ioctl(fd, BLKGETSIZE64, out_size);
		close(fd);
		if (!ret)
			return 0;
	}

	/* try mtd next */
	ret = mtd_get_meminfo(path, &meminfo);
	if (!ret) {
		*out_size = meminfo.size;
		return 0;
	}

	return ret;
}

/* Number of copies that should be allocated */
const int desired_copies = 3;

/**
 * state_storage_mtd_buckets_init - Creates storage buckets for mtd devices
 * @param storage Storage object
 * @param meminfo Info about the mtd device
 * @param path Path to the device
 * @param non_circular Use non-circular mode to write data that is compatible with the old on-flash format
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
					  const char *path, bool non_circular,
					  off_t dev_offset, size_t max_size)
{
	struct state_backend_storage_bucket *bucket;
	ssize_t end = dev_offset + max_size;
	int nr_copies = 0;
	off_t offset;

	if (!end || end > meminfo->size)
		end = meminfo->size;

	if (!IS_ALIGNED(dev_offset, meminfo->erasesize)) {
		dev_err(storage->dev, "Offset within the device is not aligned to eraseblocks. Offset is %ld, erasesize %zu\n",
			dev_offset, meminfo->erasesize);
		return -EINVAL;
	}

	for (offset = dev_offset; offset < end; offset += meminfo->erasesize) {
		int ret;
		ssize_t writesize = meminfo->writesize;
		unsigned int eraseblock = offset / meminfo->erasesize;
		bool lazy_init = true;

		if (non_circular)
			writesize = meminfo->erasesize;

		ret = state_backend_bucket_circular_create(storage->dev, path,
							   &bucket,
							   eraseblock,
							   writesize,
							   meminfo,
							   lazy_init);
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

static int state_storage_file_create(struct device_d *dev, const char *path,
				     size_t fd_size)
{
	int fd;
	uint8_t *buf;
	int ret;

	fd = open(path, O_RDWR | O_CREAT, 0600);
	if (fd < 0) {
		dev_err(dev, "Failed to open/create file '%s', %d\n", path,
			-errno);
		return -errno;
	}

	buf = xzalloc(fd_size);
	if (!buf) {
		ret = -ENOMEM;
		goto out_close;
	}

	ret = write_full(fd, buf, fd_size);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize empty file '%s', %d\n", path,
			ret);
		goto out_free;
	}
	ret = 0;

out_free:
	free(buf);
out_close:
	close(fd);
	return ret;
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
	size_t fd_size = 0;
	int ret;
	off_t offset;
	int nr_copies = 0;

	ret = state_backend_storage_get_size(path, &fd_size);
	if (ret) {
		if (ret != -ENOENT) {
			dev_err(storage->dev, "Failed to get the filesize of '%s', %d\n",
				path, ret);
			return ret;
		}
		if (!stridesize) {
			dev_err(storage->dev, "File '%s' does not exist and no information about the needed size. Please specify stridesize\n",
				path);
			return ret;
		}

		if (max_size)
			fd_size = min(dev_offset + stridesize * desired_copies,
				      dev_offset + max_size);
		else
			fd_size = dev_offset + stridesize * desired_copies;
		dev_info(storage->dev, "File '%s' does not exist, creating file of size %zd\n",
			 path, fd_size);
		ret = state_storage_file_create(storage->dev, path, fd_size);
		if (ret) {
			dev_info(storage->dev, "Failed to create file '%s', %d\n",
				 path, ret);
			return ret;
		}
	} else if (max_size) {
		fd_size = min(fd_size, (size_t)dev_offset + max_size);
	}

	if (!stridesize) {
		dev_warn(storage->dev, "WARNING, no stridesize given although we use a direct file write. Starting in degraded mode\n");
		stridesize = fd_size;
	}

	for (offset = dev_offset; offset < fd_size; offset += stridesize) {
		size_t maxsize = min((size_t)stridesize,
				     (size_t)(fd_size - offset));

		ret = state_backend_bucket_direct_create(storage->dev, path,
							 &bucket, offset,
							 maxsize);
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
		if (nr_copies >= desired_copies)
			return 0;
	}

	if (!nr_copies) {
		dev_err(storage->dev, "Failed to initialize any state direct storage bucket\n");
		return -EIO;
	}
	dev_warn(storage->dev, "Failed to initialize desired amount of direct buckets, only %d of %d succeeded\n",
		 nr_copies, desired_copies);

	return 0;
}


/**
 * state_storage_init - Init backend storage
 * @param storage Storage object
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
int state_storage_init(struct state_backend_storage *storage,
		       struct device_d *dev, const char *path,
		       off_t offset, size_t max_size, uint32_t stridesize,
		       const char *storagetype)
{
	int ret = -ENODEV;
	struct mtd_info_user meminfo;

	INIT_LIST_HEAD(&storage->buckets);
	storage->dev = dev;
	storage->name = storagetype;
	storage->stridesize = stridesize;

	if (IS_ENABLED(CONFIG_MTD))
		ret = mtd_get_meminfo(path, &meminfo);

	if (!ret && !(meminfo.flags & MTD_NO_ERASE)) {
		bool non_circular = false;
		if (!storagetype) {
			non_circular = true;
		} else if (strcmp(storagetype, "circular")) {
			dev_warn(storage->dev, "Unknown storagetype '%s', falling back to old format circular storage type.\n",
				 storagetype);
			non_circular = true;
		}
		return state_storage_mtd_buckets_init(storage, &meminfo, path,
						      non_circular, offset,
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

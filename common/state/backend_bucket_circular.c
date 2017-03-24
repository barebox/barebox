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
#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <libfile.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd-abi.h>
#include <malloc.h>
#include <mtd/mtd-peb.h>
#include <string.h>

#include "state.h"

/*
 * The circular backend bucket code. The circular backend bucket is intended
 * for mtd devices which need an erase operation.
 *
 * Erasing blocks is an operation that should be avoided. On NOR flashes erasing
 * blocks is very time consuming and on NAND flashes each block only has a limited
 * number of erase cycles allowed. For this reason we continuously write more data
 * into each eraseblock and only erase it when no more free space is available.
 * Don't confuse these multiple writes into a single eraseblock with buckets. A bucket
 * is the whole eraseblock, we just happen to reuse the same bucket for storing
 * new data.
 *
 * If your device is a mtd device, but does not have eraseblocks, like MRAMs, then
 * the direct bucket is used instead.
 */
struct state_backend_storage_bucket_circular {
	struct state_backend_storage_bucket bucket;

	unsigned int eraseblock; /* Which eraseblock is used */
	ssize_t writesize; /* Alignment of writes */
	ssize_t max_size; /* Maximum size of this bucket */

	off_t write_area; /* Start of the write area (relative offset) */
	uint32_t last_written_length; /* Size of the data written in the storage */

#ifdef __BAREBOX__
	struct mtd_info *mtd; /* mtd info (used for io in Barebox)*/
#else
	struct mtd_info_user *mtd;
	int fd;
#endif

	/* For outputs */
	struct device_d *dev;
};

struct __attribute__((__packed__)) state_backend_storage_bucket_circular_meta {
	uint32_t magic;
	uint32_t written_length;
};

static const uint32_t circular_magic = 0x14fa2d02;
static const uint8_t free_pattern = 0xff;

static inline struct state_backend_storage_bucket_circular
    *get_bucket_circular(struct state_backend_storage_bucket *bucket)
{
	return container_of(bucket,
			    struct state_backend_storage_bucket_circular,
			    bucket);
}

#ifdef __BAREBOX__
static int state_mtd_peb_read(struct state_backend_storage_bucket_circular *circ,
			      void *buf, int offset, int len)
{
	int ret;

	ret = mtd_peb_read(circ->mtd, buf, circ->eraseblock, offset, len);
	if (ret == -EBADMSG) {
		ret = mtd_peb_torture(circ->mtd, circ->eraseblock);
		if (ret == -EIO) {
			dev_err(circ->dev, "Tortured eraseblock failed and is marked bad now, PEB %u\n",
				circ->eraseblock);
			return -EIO;
		} else if (ret < 0) {
			dev_err(circ->dev, "Failed to torture eraseblock, %d\n",
				ret);
			return ret;
		}
		/*
		 * Fill with invalid data so that the next write is done
		 * behind this area
		 */
		memset(buf, 0, len);
		ret = -EUCLEAN;
		circ->write_area = 0;
		dev_dbg(circ->dev, "PEB %u has ECC error, forcing rewrite\n",
			circ->eraseblock);
	} else if (ret == -EUCLEAN) {
		dev_dbg(circ->dev, "PEB %u is unclean, forcing rewrite\n",
			circ->eraseblock);
	} else if (ret < 0) {
		dev_err(circ->dev, "Failed to read PEB %u, %d\n",
			circ->eraseblock, ret);
	}

	return ret;
}

static int state_mtd_peb_write(struct state_backend_storage_bucket_circular *circ,
			       const void *buf, int offset, int len)
{
	int ret;

	ret = mtd_peb_write(circ->mtd, buf, circ->eraseblock, offset, len);
	if (ret == -EBADMSG) {
		ret = mtd_peb_torture(circ->mtd, circ->eraseblock);
		if (ret == -EIO) {
			dev_err(circ->dev, "Tortured eraseblock failed and is marked bad now, PEB %u\n",
				circ->eraseblock);
			return -EIO;
		} else if (ret < 0) {
			dev_err(circ->dev, "Failed to torture eraseblock, %d\n",
				ret);
			return ret;
		}
		ret = -EUCLEAN;
	} else if (ret < 0 && ret != -EUCLEAN) {
		dev_err(circ->dev, "Failed to write PEB %u, %d\n",
			circ->eraseblock, ret);
	}

	return ret;
}

static int state_mtd_peb_erase(struct state_backend_storage_bucket_circular *circ)
{
	return mtd_peb_erase(circ->mtd, circ->eraseblock);
}
#else
static int state_mtd_peb_read(struct state_backend_storage_bucket_circular *circ,
			      void *buf, int suboffset, int len)
{
	int ret;
	off_t offset = suboffset;
	struct mtd_ecc_stats stat1, stat2;
	bool nostats = false;

	offset += (off_t)circ->eraseblock * circ->mtd->erasesize;

	ret = lseek(circ->fd, offset, SEEK_SET);
	if (ret < 0) {
		dev_err(circ->dev, "Failed to set circular read position to %lld, %d\n",
			offset, ret);
		return ret;
	}

	dev_dbg(circ->dev, "Read state from %ld length %zd\n", offset,
		len);

	ret = ioctl(circ->fd, ECCGETSTATS, &stat1);
	if (ret)
		nostats = true;

	ret = read_full(circ->fd, buf, len);
	if (ret < 0) {
		dev_err(circ->dev, "Failed to read circular storage len %zd, %d\n",
			len, ret);
		free(buf);
		return ret;
	}

	if (nostats)
		return 0;

	ret = ioctl(circ->fd, ECCGETSTATS, &stat2);
	if (ret)
		return 0;

	if (stat2.failed - stat1.failed > 0) {
		ret = -EUCLEAN;
		dev_dbg(circ->dev, "PEB %u has ECC error, forcing rewrite\n",
			circ->eraseblock);
	} else if (stat2.corrected - stat1.corrected > 0) {
		ret = -EUCLEAN;
		dev_dbg(circ->dev, "PEB %u is unclean, forcing rewrite\n",
			circ->eraseblock);
	}

	return ret;
}

static int state_mtd_peb_write(struct state_backend_storage_bucket_circular *circ,
			       const void *buf, int suboffset, int len)
{
	int ret;
	off_t offset = suboffset;

	offset += circ->eraseblock * circ->mtd->erasesize;

	ret = lseek(circ->fd, offset, SEEK_SET);
	if (ret < 0) {
		dev_err(circ->dev, "Failed to set position for circular write %ld, %d\n",
			offset, ret);
		return ret;
	}

	ret = write_full(circ->fd, buf, len);
	if (ret < 0) {
		dev_err(circ->dev, "Failed to write circular to %ld length %zd, %d\n",
			offset, len, ret);
		return ret;
	}

	/*
	 * We keep the fd open, so flush is necessary. We ignore the return
	 * value as flush is currently not supported for mtd under linux.
	 */
	flush(circ->fd);

	dev_dbg(circ->dev, "Written state to offset %ld length %zd data length %zd\n",
		offset, len, len);

	return 0;
}

static int state_mtd_peb_erase(struct state_backend_storage_bucket_circular *circ)
{
	return erase(circ->fd, circ->mtd->erasesize,
		     (off_t)circ->eraseblock * circ->mtd->erasesize);
}
#endif

static int state_backend_bucket_circular_read(struct state_backend_storage_bucket *bucket,
					      void ** buf_out,
					      ssize_t * len_out)
{
	struct state_backend_storage_bucket_circular *circ =
	    get_bucket_circular(bucket);
	ssize_t read_len;
	off_t offset;
	void *buf;
	int ret;

	/* Storage is empty */
	if (circ->write_area == 0)
		return -ENODATA;

	if (!circ->last_written_length) {
		/*
		 * Last write did not contain length information, assuming old
		 * state and reading from the beginning.
		 */
		offset = 0;
		read_len = min(circ->write_area, (off_t)(circ->max_size -
			       sizeof(struct state_backend_storage_bucket_circular_meta)));
		circ->write_area = 0;
		dev_dbg(circ->dev, "Detected old on-storage format\n");
	} else if (circ->last_written_length > circ->write_area
		   || !IS_ALIGNED(circ->last_written_length, circ->writesize)) {
		circ->write_area = 0;
		dev_err(circ->dev, "Error, invalid number of bytes written last time %d\n",
			circ->last_written_length);
		return -EINVAL;
	} else {
		/*
		 * Normally we read at the end of the non-free area. The length
		 * of the read is then what we read from the meta data
		 * (last_written_length)
		 */
		read_len = circ->last_written_length;
		offset = circ->write_area - read_len;
	}

	buf = xmalloc(read_len);
	if (!buf)
		return -ENOMEM;

	dev_dbg(circ->dev, "Read state from PEB %u global offset %ld length %zd\n",
		circ->eraseblock, offset, read_len);

	ret = state_mtd_peb_read(circ, buf, offset, read_len);
	if (ret < 0 && ret != -EUCLEAN) {
		dev_err(circ->dev, "Failed to read circular storage len %zd, %d\n",
			read_len, ret);
		free(buf);
		return ret;
	}

	*buf_out = buf;
	*len_out = read_len - sizeof(struct state_backend_storage_bucket_circular_meta);

	return ret;
}

static int state_backend_bucket_circular_write(struct state_backend_storage_bucket *bucket,
					       const void * buf,
					       ssize_t len)
{
	struct state_backend_storage_bucket_circular *circ =
	    get_bucket_circular(bucket);
	off_t offset;
	struct state_backend_storage_bucket_circular_meta *meta;
	uint32_t written_length = ALIGN(len + sizeof(*meta), circ->writesize);
	int ret;
	void *write_buf;

	if (written_length > circ->max_size) {
		dev_err(circ->dev, "Error, state data too big to be written, to write: %zd, writesize: %zd, length: %zd, available: %zd\n",
			written_length, circ->writesize, len, circ->max_size);
		return -E2BIG;
	}

	/*
	 * We need zero initialization so that our data comparisons don't show
	 * random changes
	 */
	write_buf = xzalloc(written_length);
	if (!write_buf)
		return -ENOMEM;

	memcpy(write_buf, buf, len);
	meta = (struct state_backend_storage_bucket_circular_meta *)
			(write_buf + written_length - sizeof(*meta));
	meta->magic = circular_magic;
	meta->written_length = written_length;

	if (circ->write_area + written_length >= circ->max_size) {
		circ->write_area = 0;
	}
	/*
	 * If the write area is at the beginning of the eraseblock, erase it and write
	 * at offset 0. As we only erase right before writing there are no
	 * conditions where we regularly erase a block multiple times without
	 * writing.
	 */
	if (circ->write_area == 0) {
		dev_dbg(circ->dev, "Erasing PEB %u\n", circ->eraseblock);
		ret = state_mtd_peb_erase(circ);
		if (ret) {
			dev_err(circ->dev, "Failed to erase PEB %u\n",
				circ->eraseblock);
			goto out_free;
		}
	}

	offset = circ->write_area;

	/*
	 * Update write_area before writing. The write operation may put
	 * arbitrary amount of the data into the storage before failing. In this
	 * case we want to start after that area.
	 */
	circ->write_area += written_length;

	ret = state_mtd_peb_write(circ, write_buf, offset, written_length);
	if (ret < 0 && ret != -EUCLEAN) {
		dev_err(circ->dev, "Failed to write circular to %ld length %zd, %d\n",
			offset, written_length, ret);
		goto out_free;
	}

	dev_dbg(circ->dev, "Written state to PEB %u offset %ld length %zd data length %zd\n",
		circ->eraseblock, offset, written_length, len);

out_free:
	free(write_buf);
	return ret;
}

/**
 * state_backend_bucket_circular_init - Initialize circular bucket
 * @param bucket
 * @return 0 on success, -errno otherwise
 *
 * This function searches for the beginning of the written area from the end of
 * the MTD device. This way it knows where the data ends and where the free area
 * starts.
 */
static int state_backend_bucket_circular_init(
		struct state_backend_storage_bucket *bucket)
{
	struct state_backend_storage_bucket_circular *circ =
	    get_bucket_circular(bucket);
	int sub_offset;
	uint32_t written_length = 0;
	uint8_t *buf;
	int ret;

	buf = xmalloc(circ->max_size);
	if (!buf)
		return -ENOMEM;

	ret = state_mtd_peb_read(circ, buf, 0, circ->max_size);
	if (ret && ret != -EUCLEAN)
		return ret;

	for (sub_offset = circ->max_size - circ->writesize; sub_offset >= 0;
	     sub_offset -= circ->writesize) {
		ret = mtd_buf_all_ff(buf + sub_offset, circ->writesize);
		if (!ret) {
			struct state_backend_storage_bucket_circular_meta *meta;

			meta = (struct state_backend_storage_bucket_circular_meta *)
					(buf + sub_offset + circ->writesize - sizeof(*meta));

			if (meta->magic != circular_magic)
				written_length = 0;
			else
				written_length = meta->written_length;

			break;
		}
	}

	circ->write_area = sub_offset + circ->writesize;
	circ->last_written_length = written_length;

	free(buf);

	return 0;
}

static void state_backend_bucket_circular_free(struct
					       state_backend_storage_bucket
					       *bucket)
{
	struct state_backend_storage_bucket_circular *circ =
	    get_bucket_circular(bucket);

	free(circ);
}

#ifdef __BAREBOX__
static int bucket_circular_is_block_bad(struct state_backend_storage_bucket_circular *circ)
{
	int ret;

	ret = mtd_peb_is_bad(circ->mtd, circ->eraseblock);
	if (ret < 0)
		dev_err(circ->dev, "Failed to determine whether eraseblock %u is bad, %d\n",
			circ->eraseblock, ret);

	return ret;
}
#else
static int bucket_circular_is_block_bad(struct state_backend_storage_bucket_circular *circ)
{
	int ret;
	loff_t offs = circ->eraseblock * circ->mtd->erasesize;

	ret = ioctl(circ->fd, MEMGETBADBLOCK, &offs);
	if (ret < 0)
		dev_err(circ->dev, "Failed to use ioctl to check for bad block at offset %ld, %d\n",
			offs, ret);

	return ret;
}
#endif

int state_backend_bucket_circular_create(struct device_d *dev, const char *path,
					 struct state_backend_storage_bucket **bucket,
					 unsigned int eraseblock,
					 ssize_t writesize,
					 struct mtd_info_user *mtd_uinfo)
{
	struct state_backend_storage_bucket_circular *circ;
	int ret;

	if (writesize < 8)
		writesize = 8;

	circ = xzalloc(sizeof(*circ));
	circ->eraseblock = eraseblock;
	circ->writesize = writesize;
	circ->max_size = mtd_uinfo->erasesize;
	circ->dev = dev;

#ifdef __BAREBOX__
	circ->mtd = mtd_uinfo->mtd;
#else
	circ->mtd = xzalloc(sizeof(*mtd_uinfo));
	memcpy(circ->mtd, mtd_uinfo, sizeof(*mtd_uinfo));
	circ->fd = open(path, O_RDWR);
	if (circ->fd < 0) {
		pr_err("Failed to open circular bucket '%s'\n", path);
		return -errno;
	}
#endif

	ret = bucket_circular_is_block_bad(circ);
	if (ret) {
		dev_info(dev, "Not using eraseblock %u, it is marked as bad (%d)\n",
			 circ->eraseblock, ret);
		ret = -EIO;
		goto out_free;
	}

	circ->bucket.read = state_backend_bucket_circular_read;
	circ->bucket.write = state_backend_bucket_circular_write;
	circ->bucket.free = state_backend_bucket_circular_free;
	*bucket = &circ->bucket;

	ret = state_backend_bucket_circular_init(*bucket);
	if (ret)
		goto out_free;

	return 0;

out_free:
#ifndef __BAREBOX__
	close(circ->fd);
#endif
	free(circ);

	return ret;
}

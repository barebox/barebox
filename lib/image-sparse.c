/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2014, The Linux Foundation. All rights reserved.
 * Portions Copyright 2014 Broadcom Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * NOTE:
 *   Although it is very similar, this license text is not identical
 *   to the "BSD-3-Clause", therefore, DO NOT MODIFY THIS LICENSE TEXT!
 */
#define pr_fmt(fmt)  "image-sparse: " fmt

#include <config.h>
#include <common.h>
#include <image-sparse.h>
#include <unistd.h>
#include <malloc.h>
#include <fs.h>
#include <libfile.h>
#include <linux/sizes.h>

#include <linux/math64.h>

#ifndef CONFIG_FASTBOOT_FLASH_FILLBUF_SIZE
#define CONFIG_FASTBOOT_FLASH_FILLBUF_SIZE (1024 * 512)
#endif

struct sparse_image_ctx {
	int fd;
	struct sparse_header sparse;
	int processed_chunks;
	struct chunk_header chunk;
	loff_t pos;
	size_t remaining;
	uint32_t fill_val;
};

int sparse_seek(struct sparse_image_ctx *si)
{
	unsigned int chunk_data_sz, payload;
	loff_t offs;
	int ret;

again:
	if (si->processed_chunks == si->sparse.total_chunks)
		return 0;

	/* Read and skip over chunk header */
	ret = read_full(si->fd, &si->chunk,
			sizeof(struct chunk_header));
	if (ret < 0)
		return ret;
	if (ret < sizeof(struct chunk_header))
		return -EINVAL;

	pr_debug("=== Chunk Header ===\n");
	pr_debug("chunk_type: 0x%x\n", si->chunk.chunk_type);
	pr_debug("chunk_data_sz: 0x%x\n", si->chunk.chunk_sz);
	pr_debug("total_size: 0x%x\n", si->chunk.total_sz);

	if (si->sparse.chunk_hdr_sz > sizeof(struct chunk_header)) {
		/*
		 * Skip the remaining bytes in a header that is longer
		 * than we expected.
		 */
		offs = lseek(si->fd, si->sparse.chunk_hdr_sz -
			 sizeof(struct chunk_header), SEEK_CUR);
		if (offs == -1)
			return -errno;
	}

	chunk_data_sz = si->sparse.blk_sz * si->chunk.chunk_sz;
	payload = si->chunk.total_sz - si->sparse.chunk_hdr_sz;

	si->processed_chunks++;

	switch (si->chunk.chunk_type) {
	case CHUNK_TYPE_RAW:
		if (payload != chunk_data_sz)
			return -EINVAL;

		si->remaining = payload;

		break;

	case CHUNK_TYPE_FILL:
		if (payload != sizeof(uint32_t))
			return -EINVAL;

		ret = read_full(si->fd, &si->fill_val, sizeof(uint32_t));
		if (ret < 0)
			return ret;
		if (ret < sizeof(uint32_t))
			return -EINVAL;

		si->remaining = chunk_data_sz;

		break;

	case CHUNK_TYPE_DONT_CARE:
		si->pos += chunk_data_sz;
		goto again;

	case CHUNK_TYPE_CRC32:
		if (payload != sizeof(uint32_t))
			return -EINVAL;

		offs = lseek(si->fd, chunk_data_sz, SEEK_CUR);
		if (offs == -1)
			return -EINVAL;
		goto again;

	default:
		pr_err("Unknown chunk type 0x%04x",
				si->chunk.chunk_type);
		return -EINVAL;
	}

	return 1;
}

loff_t sparse_image_size(struct sparse_image_ctx *si)
{
	return (loff_t)si->sparse.blk_sz * si->sparse.total_blks;
}

struct sparse_image_ctx *sparse_image_open(const char *path)
{
	struct sparse_image_ctx *si;
	loff_t offs;
	int ret;

	si = xzalloc(sizeof(*si));

	si->fd = open(path, O_RDONLY);
	if (si->fd < 0) {
		ret = -errno;
		goto out;
	}

	/* Read and skip over sparse image header */
	read(si->fd, &si->sparse, sizeof(struct sparse_header));

	if (si->sparse.file_hdr_sz > sizeof(struct sparse_header)) {
		/*
		 * Skip the remaining bytes in a header that is longer than
		 * we expected.
		 */
		offs = lseek(si->fd, si->sparse.file_hdr_sz, SEEK_SET);
		if (offs == -1) {
			ret = -errno;
			goto out;
		}
	}

	ret = sparse_seek(si);
	if (ret < 0)
		goto out;

	return si;
out:
	free(si);

	return ERR_PTR(ret);
}

int sparse_image_read(struct sparse_image_ctx *si, void *buf, loff_t *pos,
		      size_t len, int *retlen)
{
	size_t now;
	int ret, i;

	if (si->remaining == 0) {
		ret = sparse_seek(si);
		if (ret < 0)
			return ret;
		if (ret == 0) {
			*retlen = 0;
			return 0;
		}
	}

	*pos = si->pos;

	now = min(si->remaining, len);

	switch (si->chunk.chunk_type) {
	case CHUNK_TYPE_RAW:
		ret = read_full(si->fd, buf, now);
		if (ret < 0)
			return ret;
		if (ret < now)
			return -EINVAL;

		break;

	case CHUNK_TYPE_FILL:
		if (now & 3)
			return -EINVAL;

		for (i = 0; i < now / sizeof(uint32_t); i++) {
			uint32_t *buf32 = buf;

			buf32[i] = si->fill_val;
		}

		break;
	default:
		return -EINVAL;
	}

	si->pos += now;
	si->remaining -= now;

	*retlen = now;

	return 0;
}

void sparse_image_close(struct sparse_image_ctx *si)
{
	close(si->fd);
	free(si);
}

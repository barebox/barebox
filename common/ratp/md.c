/*
 * Copyright (c) 2011-2018 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <ratp_bb.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <libfile.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <xfuncs.h>

/* NOTE:
 *  - Fixed-size fields (e.g. integers) are given just after the header.
 *  - Variable-length fields are stored inside the buffer[] and their position
 *    within the buffer[] and their size are given as fixed-sized fields after
 *    the header.
 *  The message may be extended at any time keeping backwards compatibility,
 *  as the position of the buffer[] is given by the buffer_offset field. i.e.
 *  increasing the buffer_offset field we can extend the fixed-sized section
 *  to add more fields.
 */

struct ratp_bb_md_request {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint16_t addr;
	uint16_t size;
	uint16_t path_size;
	uint16_t path_offset;
	uint8_t  buffer[];
} __attribute__((packed));

struct ratp_bb_md_response {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint32_t errno;
	uint16_t data_size;
	uint16_t data_offset;
	uint8_t  buffer[];
} __attribute__((packed));

extern char *mem_rw_buf;

static int do_ratp_mem_md(const char *filename,
			  loff_t start,
			  loff_t size,
			  uint8_t *output)
{
	int r, now, t;
	int ret = 0;
	int fd;
	void *map;

	fd = open_and_lseek(filename, O_RWSIZE_1 | O_RDONLY, start);
	if (fd < 0)
		return -errno;

	map = memmap(fd, PROT_READ);
	if (map != (void *)-1) {
		memcpy(output, (uint8_t *)(map + start), size);
		goto out;
	}

	t = 0;
	do {
		now = min(size, (loff_t)RW_BUF_SIZE);
		r = read(fd, mem_rw_buf, now);
		if (r < 0) {
			ret = -errno;
			perror("read");
			goto out;
		}
		if (!r)
			goto out;

		memcpy(output + t, (uint8_t *)(mem_rw_buf), r);

		size  -= r;
		t     += r;
	} while (size);

out:
	close(fd);

	return ret;
}

static int ratp_cmd_md(const struct ratp_bb *req, int req_len,
		       struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_md_request *md_req = (struct ratp_bb_md_request *)req;
	struct ratp_bb_md_response *md_rsp;
	uint8_t *buffer;
	uint16_t buffer_offset;
	uint16_t buffer_size;
	int md_rsp_len;
	uint16_t addr;
	uint16_t size;
	uint16_t path_size;
	uint16_t path_offset;
	char *path = NULL;
	int ret = 0;

	/* At least message header should be valid */
	if (req_len < sizeof(*md_req)) {
		printf("ratp md ignored: size mismatch (%d < %zu)\n",
		       req_len, sizeof (*md_req));
		ret = -EINVAL;
		goto out;
	}

	/* Validate buffer position and size */
	buffer_offset = be16_to_cpu(md_req->buffer_offset);
	if (req_len < buffer_offset) {
		printf("ratp md ignored: invalid buffer offset (%d < %hu)\n",
		       req_len, buffer_offset);
		ret = -EINVAL;
		goto out;
	}
	buffer_size = req_len - buffer_offset;
	buffer = ((uint8_t *)md_req) + buffer_offset;

	/* Validate path position and size */
	path_offset = be16_to_cpu(md_req->path_offset);
	if (path_offset != 0) {
		printf("ratp md ignored: invalid path offset\n");
		ret = -EINVAL;
		goto out;
	}
	path_size = be16_to_cpu(md_req->path_size);
	if (!path_size) {
		printf("ratp md ignored: no filepath given\n");
		ret = -EINVAL;
		goto out;
	}

	/* Validate buffer size */
	if (buffer_size < path_size) {
		printf("ratp mw ignored: size mismatch (%d < %hu): path may not be fully given\n",
		       req_len, path_size);
		ret = -EINVAL;
		goto out;
	}

	addr = be16_to_cpu (md_req->addr);
	size = be16_to_cpu (md_req->size);
	path = xstrndup((const char *)&buffer[path_offset], path_size);

out:
	/* Avoid reading anything on error */
	if (ret != 0)
		size = 0;

	md_rsp_len = sizeof(*md_rsp) + size;
	md_rsp = xzalloc(md_rsp_len);
	md_rsp->header.type = cpu_to_be16(BB_RATP_TYPE_MD_RETURN);
	md_rsp->buffer_offset = cpu_to_be16(sizeof(*md_rsp));
	md_rsp->data_offset = 0;

	/* Don't read anything on error or if 0 bytes were requested */
	if (size > 0)
		ret = do_ratp_mem_md(path, addr, size, md_rsp->buffer);

	if (ret != 0) {
		md_rsp->data_size = 0;
		md_rsp->errno = cpu_to_be32(ret);
		md_rsp_len = sizeof(*md_rsp);
	} else {
		md_rsp->data_size = cpu_to_be16(size);
		md_rsp->errno = 0;
	}

	*rsp = (struct ratp_bb *)md_rsp;
	*rsp_len = md_rsp_len;

	free (path);
	return ret;
}

BAREBOX_RATP_CMD_START(MD)
	.request_id = BB_RATP_TYPE_MD,
	.response_id = BB_RATP_TYPE_MD_RETURN,
	.cmd = ratp_cmd_md
BAREBOX_RATP_CMD_END

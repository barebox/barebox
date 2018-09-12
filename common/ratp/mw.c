/*
 * Copyright (c) 2011-2018 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2018 Zodiac Inflight Innovations
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

#define pr_fmt(fmt) "barebox-ratp: mw: " fmt

#include <common.h>
#include <ratp_bb.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <libfile.h>
#include <fcntl.h>
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

struct ratp_bb_mw_request {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint16_t addr;
	uint16_t path_size;
	uint16_t path_offset;
	uint16_t data_size;
	uint16_t data_offset;
	uint8_t  buffer[];
} __packed;

struct ratp_bb_mw_response {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint32_t errno;
	uint16_t written;
	uint8_t  buffer[];
} __packed;

static int ratp_cmd_mw(const struct ratp_bb *req, int req_len,
		       struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_mw_request *mw_req = (struct ratp_bb_mw_request *)req;
	struct ratp_bb_mw_response *mw_rsp;
	uint8_t *buffer;
	uint16_t buffer_offset;
	uint16_t buffer_size;
	uint16_t addr;
	uint16_t path_size;
	uint16_t path_offset;
	uint16_t data_size;
	uint16_t data_offset;
	ssize_t written = 0;
	char *path = NULL;
	int fd;
	int ret = 0;

	/* At least message header should be valid */
	if (req_len < sizeof(*mw_req)) {
		pr_err("ignored: size mismatch (%d < %zu)\n",
		       req_len, sizeof(*mw_req));
		ret = -EINVAL;
		goto out;
	}

	/* Validate buffer position and size */
	buffer_offset = be16_to_cpu(mw_req->buffer_offset);
	if (req_len < buffer_offset) {
		pr_err("ignored: invalid buffer offset (%d < %hu)\n",
		       req_len, buffer_offset);
		ret = -EINVAL;
		goto out;
	}
	buffer_size = req_len - buffer_offset;
	buffer = ((uint8_t *)mw_req) + buffer_offset;

	/* Validate path position and size */
	path_offset = be16_to_cpu(mw_req->path_offset);
	if (path_offset != 0) {
		pr_err("ignored: invalid path offset\n");
		ret = -EINVAL;
		goto out;
	}
	path_size = be16_to_cpu(mw_req->path_size);
	if (!path_size) {
		pr_err("ignored: no filepath given\n");
		ret = -EINVAL;
		goto out;
	}

	/* Validate data position and size */
	data_offset = be16_to_cpu(mw_req->data_offset);
	if (data_offset != (path_offset + path_size)) {
		pr_err("ignored: invalid path offset\n");
		ret = -EINVAL;
		goto out;
	}
	data_size = be16_to_cpu(mw_req->data_size);
	if (!data_size) {
		/* Success */
		goto out;
	}

	/* Validate buffer size */
	if (buffer_size < (path_size + data_size)) {
		pr_err("ignored: size mismatch (%d < %hu): path or data not be fully given\n",
		       req_len, path_size + data_size);
		ret = -EINVAL;
		goto out;
	}

	addr = be16_to_cpu(mw_req->addr);
	path = xstrndup((const char *)&buffer[path_offset], path_size);

	fd = open_and_lseek(path, O_RWSIZE_1 | O_WRONLY, addr);
	if (fd < 0) {
		ret = -errno;
		goto out;
	}

	written = write(fd, &buffer[data_offset], data_size);
	if (written < 0) {
		ret = -errno;
		perror("write");
	}

	close(fd);

out:
	mw_rsp = xzalloc(sizeof(*mw_rsp));
	mw_rsp->header.type = cpu_to_be16(BB_RATP_TYPE_MW_RETURN);
	mw_rsp->buffer_offset = cpu_to_be16(sizeof(*mw_rsp)); /* n/a */

	if (ret != 0) {
		mw_rsp->written = 0;
		mw_rsp->errno = cpu_to_be32(ret);
	} else {
		mw_rsp->written = cpu_to_be16((uint16_t)written);
		mw_rsp->errno = 0;
	}

	*rsp = (struct ratp_bb *)mw_rsp;
	*rsp_len = sizeof(*mw_rsp);

	free(path);
	return ret;
}

BAREBOX_RATP_CMD_START(MW)
	.request_id = BB_RATP_TYPE_MW,
	.response_id = BB_RATP_TYPE_MW_RETURN,
	.cmd = ratp_cmd_mw
BAREBOX_RATP_CMD_END

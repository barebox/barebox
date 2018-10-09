/*
 * Copyright (c) 2018 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#define pr_fmt(fmt) "barebox-ratp: i2c: " fmt

#include <common.h>
#include <ratp_bb.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <i2c/i2c.h>

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

#define I2C_FLAG_WIDE_ADDRESS BIT(0)
#define I2C_FLAG_MASTER_MODE  BIT(1)

struct ratp_bb_i2c_read_request {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint8_t  bus;
	uint8_t  addr;
	uint16_t reg;
	uint8_t  flags;
	uint16_t size;
	uint8_t  buffer[];
} __packed;

struct ratp_bb_i2c_read_response {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint32_t errno;
	uint16_t data_size;
	uint16_t data_offset;
	uint8_t  buffer[];
} __packed;

struct ratp_bb_i2c_write_request {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint8_t  bus;
	uint8_t  addr;
	uint16_t reg;
	uint8_t  flags;
	uint16_t data_size;
	uint16_t data_offset;
	uint8_t  buffer[];
} __packed;

struct ratp_bb_i2c_write_response {
	struct ratp_bb header;
	uint16_t buffer_offset;
	uint32_t errno;
	uint16_t written;
	uint8_t  buffer[];
} __packed;

static int ratp_cmd_i2c_read(const struct ratp_bb *req, int req_len,
			     struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_i2c_read_request *i2c_read_req = (struct ratp_bb_i2c_read_request *)req;
	struct ratp_bb_i2c_read_response *i2c_read_rsp;
	struct i2c_adapter *adapter;
	struct i2c_client client;
	uint16_t buffer_offset;
	int i2c_read_rsp_len;
	uint16_t reg;
	uint16_t size;
	uint32_t wide = 0;
	int ret = 0;

	/* At least message header should be valid */
	if (req_len < sizeof(*i2c_read_req)) {
		pr_err("read ignored: size mismatch (%d < %zu)\n",
		       req_len, sizeof(*i2c_read_req));
		ret = -EINVAL;
		goto out_rsp;
	}

	/* We don't require any buffer here, but just validate buffer position and size */
	buffer_offset = be16_to_cpu(i2c_read_req->buffer_offset);
	if (buffer_offset != req_len) {
		pr_err("read ignored: invalid buffer offset (%hu != %d)\n",
		       buffer_offset, req_len);
		ret = -EINVAL;
		goto out_rsp;
	}

	reg  = be16_to_cpu(i2c_read_req->reg);
	size = be16_to_cpu(i2c_read_req->size);
	if (i2c_read_req->flags & I2C_FLAG_WIDE_ADDRESS)
		wide = I2C_ADDR_16_BIT;

out_rsp:
	/* Avoid reading anything on error */
	if (ret != 0)
		size = 0;

	i2c_read_rsp_len = sizeof(*i2c_read_rsp) + size;
	i2c_read_rsp = xzalloc(i2c_read_rsp_len);
	i2c_read_rsp->header.type = cpu_to_be16(BB_RATP_TYPE_I2C_READ_RETURN);
	i2c_read_rsp->buffer_offset = cpu_to_be16(sizeof(*i2c_read_rsp));
	i2c_read_rsp->data_offset = 0;

	/* Don't read anything on error or if 0 bytes were requested */
	if (size > 0) {
		adapter = i2c_get_adapter(i2c_read_req->bus);
		if (!adapter) {
			pr_err("read ignored: i2c bus %u not found\n", i2c_read_req->bus);
			ret = -ENODEV;
			goto out;
		}

		client.adapter = adapter;
		client.addr = i2c_read_req->addr;

		if (i2c_read_req->flags & I2C_FLAG_MASTER_MODE) {
			ret = i2c_master_recv(&client, i2c_read_rsp->buffer, size);
		} else {
			ret = i2c_read_reg(&client, reg | wide, i2c_read_rsp->buffer, size);
		}
		if (ret != size) {
			pr_err("read ignored: not all bytes read (%u < %u)\n", ret, size);
			ret = -EIO;
			goto out;
		}
		ret = 0;
	}

out:
	if (ret != 0) {
		i2c_read_rsp->data_size = 0;
		i2c_read_rsp->errno = cpu_to_be32(ret);
		i2c_read_rsp_len = sizeof(*i2c_read_rsp);
	} else {
		i2c_read_rsp->data_size = cpu_to_be16(size);
		i2c_read_rsp->errno = 0;
	}

	*rsp = (struct ratp_bb *)i2c_read_rsp;
	*rsp_len = i2c_read_rsp_len;

	return ret;
}

BAREBOX_RATP_CMD_START(I2C_READ)
	.request_id = BB_RATP_TYPE_I2C_READ,
	.response_id = BB_RATP_TYPE_I2C_READ_RETURN,
	.cmd = ratp_cmd_i2c_read
BAREBOX_RATP_CMD_END


static int ratp_cmd_i2c_write(const struct ratp_bb *req, int req_len,
			     struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_i2c_write_request *i2c_write_req = (struct ratp_bb_i2c_write_request *)req;
	struct ratp_bb_i2c_write_response *i2c_write_rsp;
	struct i2c_adapter *adapter;
	struct i2c_client client;
	uint8_t *buffer;
	uint16_t buffer_offset;
	uint16_t buffer_size;
	uint16_t reg;
	uint16_t data_offset;
	uint16_t data_size;
	uint32_t wide = 0;
	int ret = 0;
	int written = 0;

	/* At least message header should be valid */
	if (req_len < sizeof(*i2c_write_req)) {
		pr_err("write ignored: size mismatch (%d < %zu)\n",
		       req_len, sizeof(*i2c_write_req));
		ret = -EINVAL;
		goto out;
	}

	/* Validate buffer position and size */
	buffer_offset = be16_to_cpu(i2c_write_req->buffer_offset);
	if (req_len < buffer_offset) {
		pr_err("write ignored: invalid buffer offset (%d < %hu)\n",
		       req_len, buffer_offset);
		ret = -EINVAL;
		goto out;
	}
	buffer_size = req_len - buffer_offset;
	buffer = ((uint8_t *)i2c_write_req) + buffer_offset;

	/* Validate data position and size */
	data_offset = be16_to_cpu(i2c_write_req->data_offset);
	if (data_offset != 0) {
		pr_err("write ignored: invalid data offset\n");
		ret = -EINVAL;
		goto out;
	}
	data_size = be16_to_cpu(i2c_write_req->data_size);
	if (!data_size) {
		/* Success */
		goto out;
	}

	/* Validate buffer size */
	if (buffer_size < data_size) {
		pr_err("write ignored: size mismatch (%d < %hu): data not fully given\n",
		       buffer_size, data_size);
		ret = -EINVAL;
		goto out;
	}

	reg  = be16_to_cpu(i2c_write_req->reg);
	if (i2c_write_req->flags & I2C_FLAG_WIDE_ADDRESS)
		wide = I2C_ADDR_16_BIT;

	adapter = i2c_get_adapter(i2c_write_req->bus);
	if (!adapter) {
		pr_err("write ignored: i2c bus %u not found\n", i2c_write_req->bus);
		ret = -ENODEV;
		goto out;
	}

	client.adapter = adapter;
	client.addr = i2c_write_req->addr;

	if (i2c_write_req->flags & I2C_FLAG_MASTER_MODE) {
		written = i2c_master_send(&client, &buffer[data_offset], data_size);
	} else {
		written = i2c_write_reg(&client, reg | wide, &buffer[data_offset], data_size);
	}

	if (written != data_size) {
		pr_err("write ignored: not all bytes written (%u < %u)\n", ret, data_size);
		ret = -EIO;
		goto out;
	}

out:
	i2c_write_rsp = xzalloc(sizeof(*i2c_write_rsp));
	i2c_write_rsp->header.type = cpu_to_be16(BB_RATP_TYPE_I2C_WRITE_RETURN);
	i2c_write_rsp->buffer_offset = cpu_to_be16(sizeof(*i2c_write_rsp));  /* n/a */

	if (ret != 0) {
		i2c_write_rsp->written = 0;
		i2c_write_rsp->errno = cpu_to_be32(ret);
	} else {
		i2c_write_rsp->written = cpu_to_be16((uint16_t)written);
		i2c_write_rsp->errno = 0;
	}

	*rsp = (struct ratp_bb *)i2c_write_rsp;
	*rsp_len = sizeof(*i2c_write_rsp);

	return ret;
}

BAREBOX_RATP_CMD_START(I2C_WRITE)
	.request_id = BB_RATP_TYPE_I2C_WRITE,
	.response_id = BB_RATP_TYPE_I2C_WRITE_RETURN,
	.cmd = ratp_cmd_i2c_write
BAREBOX_RATP_CMD_END

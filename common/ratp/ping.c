/*
 * Copyright (c) 2018 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

/*
 * RATP ping
 */

#include <common.h>
#include <ratp_bb.h>
#include <malloc.h>

static int ratp_cmd_ping(const struct ratp_bb *req, int req_len,
			 struct ratp_bb **rsp, int *rsp_len)
{
	/* Just build response */
	*rsp_len = sizeof(struct ratp_bb);
	*rsp = xzalloc(*rsp_len);
	(*rsp)->type = cpu_to_be16(BB_RATP_TYPE_PONG);
	return 0;
}

BAREBOX_RATP_CMD_START(PING)
	.request_id = BB_RATP_TYPE_PING,
	.response_id = BB_RATP_TYPE_PONG,
	.cmd = ratp_cmd_ping
BAREBOX_RATP_CMD_END

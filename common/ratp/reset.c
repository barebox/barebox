/*
 * reset.c - reset the cpu
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#define pr_fmt(fmt) "barebox-ratp: reset: " fmt

#include <common.h>
#include <command.h>
#include <ratp_bb.h>
#include <complete.h>
#include <getopt.h>
#include <restart.h>

struct ratp_bb_reset {
	struct ratp_bb header;
	uint8_t        force;
} __packed;

static int ratp_cmd_reset(const struct ratp_bb *req, int req_len,
			  struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_reset *reset_req = (struct ratp_bb_reset *)req;

	if (req_len < sizeof(*reset_req)) {
		pr_err("ignored: size mismatch (%d < %zu)\n", req_len, sizeof(*reset_req));
		return 2;
	}

	pr_debug("running %s\n", reset_req->force ? "(forced)" : "");

	if (!reset_req->force)
		shutdown_barebox();

	restart_machine();
	/* Not reached */
	return 1;
}

BAREBOX_RATP_CMD_START(RESET)
	.request_id = BB_RATP_TYPE_RESET,
	.cmd = ratp_cmd_reset
BAREBOX_RATP_CMD_END

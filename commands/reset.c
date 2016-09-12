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

#include <common.h>
#include <command.h>
#include <complete.h>
#include <getopt.h>
#include <restart.h>

static int cmd_reset(int argc, char *argv[])
{
	int opt, shutdown_flag;

	shutdown_flag = 1;

	while ((opt = getopt(argc, argv, "f")) > 0) {
		switch (opt) {
		case 'f':
			shutdown_flag = 0;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (shutdown_flag)
		shutdown_barebox();

	restart_machine();

	/* Not reached */
	return 1;
}

BAREBOX_CMD_HELP_START(reset)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f",  "force RESET, don't call shutdown")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(reset)
	.cmd		= cmd_reset,
	BAREBOX_CMD_DESC("perform RESET of the CPU")
	BAREBOX_CMD_OPTS("[-f]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_reset_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

/*
 * of_fixup_status.c - Register a fixup to enable or disable nodes in the
 * device tree
 *
 * Copyright (c) 2014-2016 PHYTEC Messtechnik GmbH
 * Author:
 *	Teresa Remmet
 *	Wadim Egorov
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
 */

#include <common.h>
#include <of.h>
#include <command.h>
#include <malloc.h>
#include <complete.h>
#include <asm/byteorder.h>
#include <linux/err.h>
#include <getopt.h>
#include <string.h>

static int do_of_fixup_status(int argc, char *argv[])
{
	int opt;
	bool status = 1;
	char *node = NULL;

	while ((opt = getopt(argc, argv, "d")) > 0) {
		switch (opt) {
		case 'd':
			status = 0;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	node = xstrdup(argv[optind]);

	of_register_set_status_fixup(node, status);

	return 0;
}

BAREBOX_CMD_HELP_START(of_fixup_status)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-d",  "disable node")
BAREBOX_CMD_HELP_OPT("path",  "Node path\n")
BAREBOX_CMD_HELP_TEXT("Register a fixup to enable or disable a device tree node.")
BAREBOX_CMD_HELP_TEXT("Nodes are enabled on default. Disabled with -d.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_fixup_status)
	.cmd	= do_of_fixup_status,
	BAREBOX_CMD_DESC("register a fixup to enable or disable node")
	BAREBOX_CMD_OPTS("[-d] path")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(devicetree_file_complete)
	BAREBOX_CMD_HELP(cmd_of_fixup_status_help)
BAREBOX_CMD_END

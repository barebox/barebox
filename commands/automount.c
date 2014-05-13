/*
 * automount.c - automount devices
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>

static int do_automount(int argc, char *argv[])
{
	int opt, ret, make_dir = 0;

	while ((opt = getopt(argc, argv, "lr:d")) > 0) {
		switch (opt) {
		case 'l':
			automount_print();
			return 0;
		case 'r':
			automount_remove(optarg);
			return 0;
		case 'd':
			make_dir = 1;
			break;
		}
	}

	if (optind + 2 != argc)
		return COMMAND_ERROR_USAGE;

	if (make_dir) {
		ret = make_directory(argv[optind]);
		if (ret)
			return ret;
	}

	ret = automount_add(argv[optind], argv[optind + 1]);
	if (ret)
		printf("adding automountpoint failed: %s\n",
				strerror(-ret));

	return ret ? 1 : 0;
}

BAREBOX_CMD_HELP_START(automount)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-l", "list registered automount-points")
BAREBOX_CMD_HELP_OPT("-d", "create the mount directory")
BAREBOX_CMD_HELP_OPT("-r", "remove an automountpoint")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(automount)
	.cmd		= do_automount,
	BAREBOX_CMD_DESC("execute (mount) COMMAND when PATH ist first accessed")
	BAREBOX_CMD_OPTS("[-ldr] PATH [COMMAND]")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_automount_help)
BAREBOX_CMD_END


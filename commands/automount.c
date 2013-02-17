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
BAREBOX_CMD_HELP_USAGE("automount [OPTIONS] <PATH> <cmd>\n")
BAREBOX_CMD_HELP_SHORT("execute <cmd> when <PATH> is first accessed\n")
BAREBOX_CMD_HELP_OPT("-l", "List currently registered automountpoints\n")
BAREBOX_CMD_HELP_OPT("-d", "Create the mount path\n")
BAREBOX_CMD_HELP_OPT("-r <PATH>", "remove an automountpoint\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(automount)
	.cmd		= do_automount,
	.usage		= "automount [OPTIONS] <PATH> <cmd>",
	BAREBOX_CMD_HELP(cmd_automount_help)
BAREBOX_CMD_END


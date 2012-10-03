/*
 * mkdir.c - create directories
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
#include <fs.h>
#include <errno.h>
#include <getopt.h>

static int do_mkdir(int argc, char *argv[])
{
	int opt, parent = 0, ret;

	while((opt = getopt(argc, argv, "p")) > 0) {
		switch(opt) {
		case 'p':
			parent = 1;
			break;
		default:
			return 1;

		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	while (optind < argc) {
		if (parent) {
			ret = make_directory(argv[optind]);
			if (ret == -EEXIST)
				ret = 0;
		} else {
			ret = mkdir(argv[optind], 0);
		}
		if (ret) {
			printf("could not create %s: %s\n", argv[optind], errno_str());
			return 1;
		}
		optind++;
	}

	return 0;
}

static const __maybe_unused char cmd_mkdir_help[] =
"Usage: mkdir [directories]\n"
"Create new directories\n";

BAREBOX_CMD_START(mkdir)
	.cmd		= do_mkdir,
	.usage		= "make directories",
	BAREBOX_CMD_HELP(cmd_mkdir_help)
BAREBOX_CMD_END

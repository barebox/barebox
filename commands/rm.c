/*
 * rm.c - remove files
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
#include <getopt.h>
#include <errno.h>

static int do_rm(int argc, char *argv[])
{
	int i, opt, recursive = 0;

	while ((opt = getopt(argc, argv, "r")) > 0) {
		switch (opt) {
		case 'r':
			recursive = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	i = optind;

	while (i < argc) {
		int ret;

		if (recursive)
			ret = unlink_recursive(argv[i], NULL);
		else
			ret = unlink(argv[i]);
		if (ret) {
			printf("could not remove %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}

static const __maybe_unused char cmd_rm_help[] =
"Usage: rm [OPTIONS] [FILES]\n"
"Remove files\n"
"-r  remove directories and their contents recursively\n";

BAREBOX_CMD_START(rm)
	.cmd		= do_rm,
	.usage		= "remove files",
	BAREBOX_CMD_HELP(cmd_rm_help)
BAREBOX_CMD_END

/*
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
#include <errno.h>
#include <getopt.h>
#include <fs.h>
#include <fcntl.h>
#include <envfs.h>

static int do_saveenv(int argc, char *argv[])
{
	int ret, opt;
	unsigned envfs_flags = 0;
	char *filename = NULL, *dirname = NULL;

	printf("saving environment\n");
	while ((opt = getopt(argc, argv, "z")) > 0) {
		switch (opt) {
		case 'z':
			envfs_flags |= ENVFS_FLAGS_FORCE_BUILT_IN;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	/* destination and source are given? */
	if (argc - optind > 1)
		dirname = argv[optind + 1];

	/* destination only given? */
	if (argc - optind > 0)
		filename = argv[optind];

	ret = envfs_save(filename, dirname, envfs_flags);

	return ret;
}

BAREBOX_CMD_HELP_START(saveenv)
BAREBOX_CMD_HELP_TEXT("Save the files in DIRECTORY to the persistent storage device ENVFS.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("ENVFS is usually a block in flash but can be any other file. If")
BAREBOX_CMD_HELP_TEXT("omitted, DIRECTORY defaults to /env and ENVFS defaults to")
BAREBOX_CMD_HELP_TEXT("/dev/env0.")
BAREBOX_CMD_HELP_OPT ("-z",  "force the built-in default environment at startup")

BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(saveenv)
	.cmd		= do_saveenv,
	BAREBOX_CMD_DESC("save environment to persistent storage")
	BAREBOX_CMD_OPTS("[-z] [ENVFS [DIRECTORY]]")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_saveenv_help)
BAREBOX_CMD_END

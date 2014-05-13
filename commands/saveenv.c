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

/**
 * @file
 * @brief saveenv: Make the environment persistent
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <envfs.h>

static int do_saveenv(int argc, char *argv[])
{
	int ret;
	char *filename, *dirname;

	printf("saving environment\n");
	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = default_environment_path_get();
	else
		filename = argv[1];

	ret = envfs_save(filename, dirname);

	return ret;
}

BAREBOX_CMD_HELP_START(saveenv)
BAREBOX_CMD_HELP_TEXT("Save the files in DIRECTORY to the persistent storage device ENVFS.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("ENVFS is usually a block in flash but can be any other file. If")
BAREBOX_CMD_HELP_TEXT("omitted, DIRECTORY defaults to /env and ENVFS defaults to")
BAREBOX_CMD_HELP_TEXT("/dev/env0. Note that envfs can only handle files, directories are being")
BAREBOX_CMD_HELP_TEXT("skipped silently.")

BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(saveenv)
	.cmd		= do_saveenv,
	BAREBOX_CMD_DESC("save environment to persistent storage")
	BAREBOX_CMD_OPTS("[ENVFS] [DIRECTORY]")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_saveenv_help)
BAREBOX_CMD_END

/**
 * @page saveenv_command

<p>\<envfs> is usually a block in flash but can be any other file. If
omitted, \<directory> defaults to /env and \<envfs> defaults to
/dev/env0. Note that envfs can only handle files, directories are being
skipped silently.</p>

\todo What does 'block in flash' mean? Add example.

 */


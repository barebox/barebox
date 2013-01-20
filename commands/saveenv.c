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
	int ret, fd;
	char *filename, *dirname;

	printf("saving environment\n");
	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = default_environment_path;
	else
		filename = argv[1];

	fd = open(filename, O_WRONLY | O_CREAT);
	if (fd < 0) {
		printf("could not open %s: %s\n", filename, errno_str());
		return 1;
	}

	ret = protect(fd, ~0, 0, 0);

	/* ENOSYS is no error here, many devices do not need it */
	if (ret && errno != ENOSYS) {
		printf("could not unprotect %s: %s\n", filename, errno_str());
		close(fd);
		return 1;
	}

	ret = erase(fd, ~0, 0);

	/* ENOSYS is no error here, many devices do not need it */
	if (ret && errno != ENOSYS) {
		printf("could not erase %s: %s\n", filename, errno_str());
		close(fd);
		return 1;
	}

	close(fd);

	ret = envfs_save(filename, dirname);
	if (ret) {
		printf("saveenv failed\n");
		goto out;
	}

	fd = open(filename, O_WRONLY | O_CREAT);

	ret = protect(fd, ~0, 0, 1);

	/* ENOSYS is no error here, many devices do not need it */
	if (ret && errno != ENOSYS) {
		printf("could not protect %s: %s\n", filename, errno_str());
		close(fd);
		return 1;
	}

	ret = 0;
out:
	close(fd);
	return ret;
}

BAREBOX_CMD_HELP_START(saveenv)
BAREBOX_CMD_HELP_USAGE("saveenv [envfs] [directory]\n")
BAREBOX_CMD_HELP_SHORT("Save the files in <directory> to the persistent storage device <envfs>.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(saveenv)
	.cmd		= do_saveenv,
	.usage		= "save environment to persistent storage",
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


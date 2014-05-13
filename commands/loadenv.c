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
 * @brief loadenv: Restoring a environment
 */

#include <common.h>
#include <getopt.h>
#include <command.h>
#include <envfs.h>
#include <errno.h>
#include <fs.h>
#include <malloc.h>

static int do_loadenv(int argc, char *argv[])
{
	char *filename = NULL, *dirname;
	unsigned flags = 0;
	int opt;
	int scrub = 0;
	int defaultenv = 0;

	while ((opt = getopt(argc, argv, "nsd")) > 0) {
		switch (opt) {
		case 'n':
			flags |= ENV_FLAG_NO_OVERWRITE;
			break;
		case 's':
			scrub = 1;
			break;
		case 'd':
			defaultenv = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc - optind < 2)
		dirname = "/env";
	else
		dirname = argv[optind + 1];

	if (argc - optind < 1) {
		filename = default_environment_path_get();
	} else {
		char *str = normalise_path(argv[optind]);

		/*
		 * /dev/defaultenv use to contain the defaultenvironment.
		 * we do not have this file anymore, but maintain compatibility
		 * to the 'loadenv -s /dev/defaultenv' command to restore the
		 * default environment for some time.
		 */
		if (!strcmp(str, "/dev/defaultenv"))
			defaultenv = 1;
		else
			filename = argv[optind];

		free(str);
	}

	if (scrub) {
		int ret;

		ret = unlink_recursive(dirname, NULL);
		if (ret && ret != -ENOENT) {
			eprintf("cannot remove %s: %s\n", dirname,
					strerror(-ret));
			return 1;
		}

		ret = mkdir(dirname, 0);
		if (ret) {
			eprintf("cannot create %s: %s\n", dirname,
					strerror(-ret));
			return ret;
		}
	}

	printf("loading environment from %s\n", defaultenv ? "defaultenv" : filename);

	if (defaultenv)
		return defaultenv_load(dirname, flags);
	else
		return envfs_load(filename, dirname, flags);
}

BAREBOX_CMD_HELP_START(loadenv)
BAREBOX_CMD_HELP_TEXT("Load environment from files in ENVFS (default /dev/env0) in")
BAREBOX_CMD_HELP_TEXT("DIRECTORY (default /env")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-n", "do not overwrite existing files")
BAREBOX_CMD_HELP_OPT("-s", "scrub old environment")
BAREBOX_CMD_HELP_OPT("-d", "load default environment")
BAREBOX_CMD_HELP_END

/**
 * @page loadenv_command

ENVFS can only handle files, directories are skipped silently.

\todo This needs proper documentation. What is ENVFS, why is it FS etc. Explain the concepts.

 */

BAREBOX_CMD_START(loadenv)
	.cmd		= do_loadenv,
	BAREBOX_CMD_DESC("load environment from ENVFS")
	BAREBOX_CMD_OPTS("{-nsd] [ENVFS] [DIRECTORY]")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_loadenv_help)
BAREBOX_CMD_END

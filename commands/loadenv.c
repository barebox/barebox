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
#include <fs.h>

static int do_loadenv(int argc, char *argv[])
{
	char *filename, *dirname;
	unsigned flags = 0;
	int opt;
	int scrub = 0;

	while ((opt = getopt(argc, argv, "ns")) > 0) {
		switch (opt) {
		case 'n':
			flags |= ENV_FLAG_NO_OVERWRITE;
			break;
		case 's':
			scrub = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc - optind < 2)
		dirname = "/env";
	else
		dirname = argv[optind + 1];

	if (argc - optind < 1)
		filename = default_environment_path;
	else
		filename = argv[optind];

	if (scrub) {
		int ret;

		ret = unlink_recursive(dirname, NULL);
		if (ret) {
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

	printf("loading environment from %s\n", filename);

	return envfs_load(filename, dirname, flags);
}

BAREBOX_CMD_HELP_START(loadenv)
BAREBOX_CMD_HELP_USAGE("loadenv OPTIONS [ENVFS] [DIRECTORY]\n")
BAREBOX_CMD_HELP_OPT("-n", "do not overwrite existing files\n")
BAREBOX_CMD_HELP_OPT("-s", "scrub old environment\n")
BAREBOX_CMD_HELP_SHORT("Load environment from ENVFS into DIRECTORY (default: /dev/env0 -> /env).\n")
BAREBOX_CMD_HELP_END

/**
 * @page loadenv_command

ENVFS can only handle files, directories are skipped silently.

\todo This needs proper documentation. What is ENVFS, why is it FS etc. Explain the concepts.

 */

BAREBOX_CMD_START(loadenv)
	.cmd		= do_loadenv,
	.usage		= "Load environment from ENVFS into DIRECTORY (default: /dev/env0 -> /env).",
	BAREBOX_CMD_HELP(cmd_loadenv_help)
BAREBOX_CMD_END

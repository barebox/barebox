/*
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <getopt.h>
#include <command.h>
#include <envfs.h>
#include <errno.h>
#include <fs.h>
#include <libfile.h>
#include <malloc.h>
#include <globalvar.h>

static int do_defaultenv(int argc, char *argv[])
{
	char *dirname;
	int opt, ret;
	char *restorepath = "/";
	char *from, *to;
	int restore = 0, scrub = 0;

	while ((opt = getopt(argc, argv, "p:rs")) > 0) {
		switch (opt) {
		case 'r':
			restore = 1;
			break;
		case 'p':
			restorepath = optarg;
			break;
		case 's':
			scrub = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!restore || argc != optind)
		return COMMAND_ERROR_USAGE;

	dirname = "/env";

	make_directory(dirname);

	ret = defaultenv_load("/.defaultenv", 0);
	if (ret)
		return ret;

	from = basprintf("/.defaultenv/%s", restorepath);
	to = basprintf("%s/%s", dirname, restorepath);

	printf("Restoring %s from default environment\n", restorepath);

	if (scrub)
		unlink_recursive(to, NULL);

	ret = copy_recursive(from, to);
	free(from);
	free(to);

	nvvar_load();

	unlink_recursive("/.defaultenv", NULL);

	return ret;
}

BAREBOX_CMD_HELP_START(defaultenv)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-r", "restore default environment")
BAREBOX_CMD_HELP_OPT("-s", "scrub, remove files not in default environment")
BAREBOX_CMD_HELP_OPT("-p <PATH>", "limit to <PATH>")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(defaultenv)
	.cmd		= do_defaultenv,
	BAREBOX_CMD_DESC("Restore environment from default environment")
	BAREBOX_CMD_OPTS("[-rs] [-p <PATH>]")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_defaultenv_help)
BAREBOX_CMD_END

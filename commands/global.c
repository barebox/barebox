/*
 * global.c - global shell variables
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
 *
 */
#include <common.h>
#include <malloc.h>
#include <command.h>
#include <globalvar.h>
#include <environment.h>
#include <getopt.h>
#include <complete.h>

static int do_global(int argc, char *argv[])
{
	int opt, i;
	int do_remove = 0;
	char *value;

	while ((opt = getopt(argc, argv, "r")) > 0) {
		switch (opt) {
		case 'r':
			do_remove = 1;
			break;
		}
	}

	if (argc == optind) {
		globalvar_print();
		return 0;
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
		return COMMAND_ERROR_USAGE;

	for (i = 0; i < argc; i++) {
		value = strchr(argv[i], '=');
		if (value) {
			*value = 0;
			value++;
		}

		if (do_remove)
			globalvar_remove(argv[i]);
		else
			globalvar_add_simple(argv[i], value);
	}

	return 0;
}

BAREBOX_CMD_HELP_START(global)
BAREBOX_CMD_HELP_TEXT("Add a new global variable named VAR, optionally set to VALUE.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-r", "Remove globalvars")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(global)
	.cmd		= do_global,
	BAREBOX_CMD_DESC("create or set global variables")
	BAREBOX_CMD_OPTS("[-r] VAR[=VALUE] ...")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_global_help)
	BAREBOX_CMD_COMPLETE(global_complete)
BAREBOX_CMD_END

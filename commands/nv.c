/*
 * nv.c - non volatile shell variables
 *
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
#include <malloc.h>
#include <command.h>
#include <globalvar.h>
#include <environment.h>
#include <getopt.h>

static int do_nv(int argc, char *argv[])
{
	int opt;
	int do_remove = 0;
	int ret;
	char *value;

	while ((opt = getopt(argc, argv, "r")) > 0) {
		switch (opt) {
		case 'r':
			do_remove = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc == optind) {
		nvvar_print();
		return 0;
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	value = strchr(argv[0], '=');
	if (value) {
		*value = 0;
		value++;
	}

	if (do_remove)
		ret = nvvar_remove(argv[0]);
	else
		ret = nvvar_add(argv[0], value);

	return ret;
}

BAREBOX_CMD_HELP_START(nv)
BAREBOX_CMD_HELP_TEXT("Add a new non volatile variable named VAR, optionally set to VALUE.")
BAREBOX_CMD_HELP_TEXT("non volatile variables are persistent variables that overwrite the")
BAREBOX_CMD_HELP_TEXT("global variables of the same name. Their value is saved with")
BAREBOX_CMD_HELP_TEXT("'saveenv'.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-r", "remove a non volatile variable")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(nv)
	.cmd		= do_nv,
	BAREBOX_CMD_DESC("create or set non volatile variables")
	BAREBOX_CMD_OPTS("[-r] VAR[=VALUE]")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_nv_help)
BAREBOX_CMD_END

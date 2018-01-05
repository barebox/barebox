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
#include <complete.h>

static int do_nv(int argc, char *argv[])
{
	int opt;
	int do_remove = 0, do_save = 0;
	int failed = 0, i;
	char *value;

	while ((opt = getopt(argc, argv, "rs")) > 0) {
		switch (opt) {
		case 'r':
			do_remove = 1;
			break;
		case 's':
			do_save = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc == 1) {
		nvvar_print();
		return 0;
	}

	if (do_save) {
		return nvvar_save();
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		printf("Invalid usage: Missing argument");
		return COMMAND_ERROR_USAGE;
	}

	for (i = 0; i < argc; i++) {
		int ret;
		value = strchr(argv[0], '=');
		if (value) {
			*value = 0;
			value++;
		}

		if (do_remove) {
			ret = nvvar_remove(argv[i]);
			if (ret) {
				printf("Failed removing %s: %s\n", argv[i], strerror(-ret));
				failed = 1;
			}
		} else {
			ret = nvvar_add(argv[i], value);
			if (ret) {
				printf("Failed adding %s: %s\n", argv[i], strerror(-ret));
				failed = 1;
			}
		}
	}

	return failed;
}

BAREBOX_CMD_HELP_START(nv)
BAREBOX_CMD_HELP_TEXT("Add a new non volatile (NV) variable named VAR, optionally set to VALUE.")
BAREBOX_CMD_HELP_TEXT("NV variables are persistent variables that overwrite the global variables")
BAREBOX_CMD_HELP_TEXT("of the same name.")
BAREBOX_CMD_HELP_TEXT("Their value is saved implicitly with 'saveenv' or explicitly with 'nv -s'")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-r VAR1 ...", "remove NV variable(s)")
BAREBOX_CMD_HELP_OPT("-s\t", "save NV variables")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(nv)
	.cmd		= do_nv,
	BAREBOX_CMD_DESC("create, set or remove non volatile (NV) variables")
	BAREBOX_CMD_OPTS("[-r] VAR[=VALUE] ...")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_nv_help)
	BAREBOX_CMD_COMPLETE(nv_complete)
BAREBOX_CMD_END

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <command.h>
#include <common.h>
#include <getopt.h>
#include <menu.h>
#include <password.h>

static int do_menutree(int argc, char *argv[])
{
	int opt, ret;
	char *path = "/env/menu";

	login();

	while ((opt = getopt(argc, argv, "m:")) > 0) {
		switch (opt) {
		case 'm':
			path = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	ret = menutree(path, 1);

	return ret;
}

BAREBOX_CMD_HELP_START(menutree)
BAREBOX_CMD_HELP_TEXT("Each menu entry is described by a subdirectory. Each subdirectory")
BAREBOX_CMD_HELP_TEXT("can contain the following files which further describe the entry:")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("title  A file containing the title of the entry as shown in the menu")
BAREBOX_CMD_HELP_TEXT("box    If present, the entry is a 'bool' entry. The file contains a")
BAREBOX_CMD_HELP_TEXT("       name from which the current state of the bool is taken from and saved")
BAREBOX_CMD_HELP_TEXT("       to.")
BAREBOX_CMD_HELP_TEXT("action if present this file contains a shell script which is executed when")
BAREBOX_CMD_HELP_TEXT("       when the entry is selected.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("If neither 'box' or 'action' are present, this entry is considered a submenu")
BAREBOX_CMD_HELP_TEXT("containing more entries.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-m DIR", "directory where the menu starts (Default: /env/menu)")
BAREBOX_CMD_HELP_END


BAREBOX_CMD_START(menutree)
	.cmd	= do_menutree,
	BAREBOX_CMD_DESC("create menu from directory structure")
	BAREBOX_CMD_OPTS("[-m] DIR")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_menutree_help)
BAREBOX_CMD_END

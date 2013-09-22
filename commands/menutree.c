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

static int do_menutree(int argc, char *argv[])
{
	int opt, ret;
	char *path = "/env/menu";

	while ((opt = getopt(argc, argv, "m:")) > 0) {
		switch (opt) {
		case 'm':
			path = optarg;
			break;
		}
	}

	ret = menutree(path, 1);

	return ret;
}

BAREBOX_CMD_HELP_START(menutree)
BAREBOX_CMD_HELP_USAGE("menutree [OPTIONS]\n")
"\n"
"Create a menu from a directory structure\n"
"Each menu entry is described by a subdirectory. Each subdirectory\n"
"can contain the following files which further describe the entry:\n"
"\n"
"title -  A file containing the title of the entry as shown in the menu\n"
"box -    If present, the entry is a 'bool' entry. The file contains a variable\n"
"         name from which the current state of the bool is taken from and saved\n"
"         to.\n"
"action - if present this file contains a shell script which is executed when\n"
"         when the entry is selected.\n"
"If neither 'box' or 'action' are present this entry is considered a submenu\n"
"containing more entries.\n"
"\n"
"Options:\n"
" -m <dir>     directory where the menu starts (/env/menu)\n"

BAREBOX_CMD_HELP_END


BAREBOX_CMD_START(menutree)
	.cmd	= do_menutree,
	.usage		= "create a menu from a directory structure",
	BAREBOX_CMD_HELP(cmd_menutree_help)
BAREBOX_CMD_END

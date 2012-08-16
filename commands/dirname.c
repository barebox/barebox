/*
 * dirname.c - strip directory and suffix from filenames
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <libgen.h>
#include <environment.h>
#include <fs.h>
#include <getopt.h>

static int do_dirname(int argc, char *argv[])
{
	int opt;
	int path_fs = 0;
	int len = 0;

	while ((opt = getopt(argc, argv, "V")) > 0) {
		switch (opt) {
		case 'V':
			path_fs = 1;
			break;
		}
	}

	if (argc < optind + 2)
		return COMMAND_ERROR_USAGE;

	if (path_fs)
		len = strlen(get_mounted_path(argv[optind]));

	setenv(argv[optind + 1], dirname(argv[optind]) + len);

	return 0;
}

BAREBOX_CMD_HELP_START(dirname)
BAREBOX_CMD_HELP_USAGE("dirname [-V] NAME DIRNAME\n")
BAREBOX_CMD_HELP_SHORT("strip last componext of NAME and store into $DIRNAME\n")
BAREBOX_CMD_HELP_SHORT("-V return the path relative to the mountpoint.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dirname)
	.cmd		= do_dirname,
	.usage		= "strip last component from file name",
	BAREBOX_CMD_HELP(cmd_dirname_help)
BAREBOX_CMD_END

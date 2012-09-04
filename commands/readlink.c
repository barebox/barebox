/*
 * readlink.c - read value of a symbolic link
 *
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <malloc.h>
#include <getopt.h>

static int do_readlink(int argc, char *argv[])
{
	char realname[PATH_MAX];
	int canonicalize = 0;
	int opt;

	memset(realname, 0, PATH_MAX);

	while ((opt = getopt(argc, argv, "f")) > 0) {
		switch (opt) {
		case 'f':
			canonicalize = 1;
			break;
		}
	}

	if (argc < optind + 2)
		return COMMAND_ERROR_USAGE;

	if (readlink(argv[optind], realname, PATH_MAX - 1) < 0)
		goto err;

	if (canonicalize) {
		char *buf = normalise_link(argv[optind], realname);

		if (!buf)
			goto err;
		setenv(argv[optind + 1], buf);
		free(buf);
	} else {
		setenv(argv[optind + 1], realname);
	}

	return 0;
err:
	setenv(argv[optind + 1], "");
	return 1;
}

BAREBOX_CMD_HELP_START(readlink)
BAREBOX_CMD_HELP_USAGE("readlink [-f] FILE REALNAME\n")
BAREBOX_CMD_HELP_SHORT("read value of a symbolic link and store into $REALNAME\n")
BAREBOX_CMD_HELP_SHORT("-f canonicalize by following first symlink");
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(readlink)
	.cmd		= do_readlink,
	.usage		= "read value of a symbolic link",
	BAREBOX_CMD_HELP(cmd_readlink_help)
BAREBOX_CMD_END

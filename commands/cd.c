/*
 * cd.c - change working directory
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file
 * @brief Change working directory
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_cd(struct command *cmdtp, int argc, char *argv[])
{
	int ret;

	if (argc == 1)
		ret = chdir("/");
	else
		ret = chdir(argv[1]);

	if (ret) {
		perror("chdir");
		return 1;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(cd)
BAREBOX_CMD_HELP_USAGE("cd [directory]\n")
BAREBOX_CMD_HELP_SHORT("Change to directory.\n")
BAREBOX_CMD_HELP_TEXT ("If called without an argument, change to the root directory /.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(cd)
	.cmd		= do_cd,
	.usage		= "change working directory",
	BAREBOX_CMD_HELP(cmd_cd_help)
BAREBOX_CMD_END

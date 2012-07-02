/*
 * basename.c - strip directory and suffix from filenames
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

static int do_basename(int argc, char *argv[])
{
	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	setenv(argv[2], basename(argv[1]));

	return 0;
}

BAREBOX_CMD_HELP_START(basename)
BAREBOX_CMD_HELP_USAGE("basename NAME BASENAME\n")
BAREBOX_CMD_HELP_SHORT("strip NAME and store into $BASENAME\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(basename)
	.cmd		= do_basename,
	.usage		= "strip directory and suffix from filenames",
	BAREBOX_CMD_HELP(cmd_basename_help)
BAREBOX_CMD_END

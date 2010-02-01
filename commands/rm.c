/*
 * rm.c - remove files
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
#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_rm(struct command *cmdtp, int argc, char *argv[])
{
	int i = 1;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while (i < argc) {
		if (unlink(argv[i])) {
			printf("could not remove %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}

static const __maybe_unused char cmd_rm_help[] =
"Usage: rm [FILES]\n"
"Remove files\n";

BAREBOX_CMD_START(rm)
	.cmd		= do_rm,
	.usage		= "remove files",
	BAREBOX_CMD_HELP(cmd_rm_help)
BAREBOX_CMD_END

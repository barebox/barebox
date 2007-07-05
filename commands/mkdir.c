/*
 * mkdir.c - create directories
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

static int do_mkdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 1;

	if (argc < 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	while (i < argc) {
		if (mkdir(argv[i])) {
			printf("could not create %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}

static __maybe_unused char cmd_mkdir_help[] =
"Usage: mkdir [directories]\n"
"Create new directories\n";

U_BOOT_CMD_START(mkdir)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mkdir,
	.usage		= "make directories",
	U_BOOT_CMD_HELP(cmd_mkdir_help)
U_BOOT_CMD_END

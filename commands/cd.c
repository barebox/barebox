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

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_cd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

static __maybe_unused char cmd_cd_help[] =
"Usage: cd [directory]\n"
"change to directory. If called without argument, change to /\n";

U_BOOT_CMD_START(cd)
	.maxargs	= 2,
	.cmd		= do_cd,
	.usage		= "change working directory",
	U_BOOT_CMD_HELP(cmd_cd_help)
U_BOOT_CMD_END

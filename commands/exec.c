/*
 * exec.c - execute scripts
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
#include <fcntl.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <xfuncs.h>

static int do_exec(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i;
	char *script;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	for (i=1; i<argc; ++i) {
		script = read_file(argv[i]);
		if (!script)
			return 1;

		if (run_command (script, flag) == -1)
			goto out;
		free(script);
	}
	return 0;

out:
	free(script);
	return 1;
}

U_BOOT_CMD_START(exec)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_exec,
	.usage		= "execute a script",
U_BOOT_CMD_END

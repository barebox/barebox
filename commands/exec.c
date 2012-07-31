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
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <xfuncs.h>

static int do_exec(int argc, char *argv[])
{
	int i;
	char *script;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	for (i=1; i<argc; ++i) {
		script = read_file(argv[i], NULL);
		if (!script)
			return 1;

		if (run_command (script, 0) == -1)
			goto out;
		free(script);
	}
	return 0;

out:
	free(script);
	return 1;
}

BAREBOX_CMD_START(exec)
	.cmd		= do_exec,
	.usage		= "execute a script",
BAREBOX_CMD_END

// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* exec.c - execute scripts */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <errno.h>
#include <libfile.h>
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

		if (run_command(script) == -1)
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
	BAREBOX_CMD_DESC("execute a script")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
BAREBOX_CMD_END

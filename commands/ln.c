// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>

/* ln.c - make links between files */

#include <common.h>
#include <command.h>
#include <libgen.h>
#include <environment.h>
#include <fs.h>
#include <errno.h>

static int do_ln(int argc, char *argv[])
{
	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	if (symlink(argv[1], argv[2]) < 0) {
		perror("ln");
		return 1;
	}
	return 0;
}

BAREBOX_CMD_START(ln)
	.cmd		= do_ln,
	BAREBOX_CMD_DESC("create symlink (make a new name for a file)")
	BAREBOX_CMD_OPTS("SRC DEST")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
BAREBOX_CMD_END

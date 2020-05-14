// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* meminfo.c - show information about memory usage */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <malloc.h>

static int do_meminfo(int argc, char *argv[])
{
	malloc_stats();

	return 0;
}

BAREBOX_CMD_START(meminfo)
	.cmd		= do_meminfo,
	BAREBOX_CMD_DESC("print info about memory usage")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

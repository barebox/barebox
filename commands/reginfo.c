// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* reginfo.c - print information about SoC specifc registers */

#include <common.h>
#include <command.h>
#include <complete.h>

static int do_reginfo(int argc, char *argv[])
{
	reginfo();
	return 0;
}

BAREBOX_CMD_START(reginfo)
	.cmd		= do_reginfo,
	BAREBOX_CMD_DESC("print register information")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

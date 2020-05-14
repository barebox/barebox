// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* regulator command */

#include <common.h>
#include <command.h>
#include <regulator.h>

static int do_regulator(int argc, char *argv[])
{
	regulators_print();

	return 0;
}

BAREBOX_CMD_START(regulator)
	.cmd		= do_regulator,
	BAREBOX_CMD_DESC("list regulators")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END

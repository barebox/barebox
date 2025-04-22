// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2011 Antony Pavlov <antonynpavlov@gmail.com>

/* poweroff.c - turn board's power off */

#include <common.h>
#include <command.h>
#include <poweroff.h>

static int cmd_poweroff(int argc, char *argv[])
{
	poweroff_machine(0);

	/* Not reached */
	return 1;
}

BAREBOX_CMD_START(poweroff)
	.cmd		= cmd_poweroff,
	BAREBOX_CMD_DESC("turn the power off")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

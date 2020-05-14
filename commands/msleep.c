// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>, Pengutronix

/*
 * msleep.c - delay execution for n milliseconds
 *
 * derived from commands/sleep.c
 */

#include <common.h>
#include <command.h>
#include <clock.h>

static int do_msleep(int argc, char *argv[])
{
	ulong delay;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	delay = simple_strtoul(argv[1], NULL, 10);

	mdelay(delay);

	return 0;
}

BAREBOX_CMD_START(msleep)
	.cmd		= do_msleep,
	BAREBOX_CMD_DESC("delay execution for n milli-seconds")
	BAREBOX_CMD_OPTS("MILLISECONDS")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
BAREBOX_CMD_END

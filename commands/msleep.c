/*
 * msleep.c - delay execution for n milliseconds
 *
 * Copyright (c) 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>, Pengutronix
 *
 * derived from commands/sleep.c
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
	.usage		= "delay execution for n milliseconds",
BAREBOX_CMD_END

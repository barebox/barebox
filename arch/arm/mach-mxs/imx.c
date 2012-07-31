/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>

extern void imx_dump_clocks(void);

static int do_clocks(int argc, char *argv[])
{
	imx_dump_clocks();

	return 0;
}

BAREBOX_CMD_START(dump_clocks)
	.cmd		= do_clocks,
	.usage		= "show clock frequencies",
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

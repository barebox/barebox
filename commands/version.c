/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

static int do_version(int argc, char *argv[])
{
	printf ("\n%s\n", version_string);
	return 0;
}

BAREBOX_CMD_START(version)
	.cmd		= do_version,
	.usage		= "print monitor version",
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END


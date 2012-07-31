/*
 * meminfo.c - show information about memory usage
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <complete.h>
#include <malloc.h>

static int do_meminfo(int argc, char *argv[])
{
	malloc_stats();

	return 0;
}

BAREBOX_CMD_START(meminfo)
	.cmd		= do_meminfo,
	.usage		= "print info about memory usage",
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

/*
 * poweroff.c - turn board's power off
 *
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
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
#include <poweroff.h>

static int cmd_poweroff(int argc, char *argv[])
{
	poweroff_machine();

	/* Not reached */
	return 1;
}

BAREBOX_CMD_START(poweroff)
	.cmd		= cmd_poweroff,
	BAREBOX_CMD_DESC("turn the power off")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

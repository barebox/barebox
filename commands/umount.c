/*
 * umount.c - umount filesystems
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
#include <fs.h>
#include <errno.h>

static int do_umount(int argc, char *argv[])
{
	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	return umount(argv[1]);
}


BAREBOX_CMD_HELP_START(umount)
BAREBOX_CMD_HELP_TEXT("Unmount a filesystem mounted on a specific MOINTPOINT")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(umount)
	.cmd		= do_umount,
	BAREBOX_CMD_DESC("umount a filesystem")
	BAREBOX_CMD_OPTS("MOUNTPOINT")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_umount_help)
BAREBOX_CMD_END

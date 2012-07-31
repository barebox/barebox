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
	int ret = 0;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	if ((ret = umount(argv[1]))) {
		perror("umount");
		return 1;
	}
	return 0;
}

static const __maybe_unused char cmd_umount_help[] =
"Usage: umount <mountpoint>\n"
"umount a filesystem mounted on a specific mountpoint\n";

BAREBOX_CMD_START(umount)
	.cmd		= do_umount,
	.usage		= "umount a filesystem",
	BAREBOX_CMD_HELP(cmd_umount_help)
BAREBOX_CMD_END

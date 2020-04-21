// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* umount.c - umount filesystems */

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
	BAREBOX_CMD_OPTS("MOUNTPOINT/DEVICEPATH")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_umount_help)
BAREBOX_CMD_END

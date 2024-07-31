// SPDX-License-Identifier: GPL-2.0-or-later

/* sync - flushing support */

#include <common.h>
#include <command.h>
#include <fcntl.h>
#include <fs.h>

static int do_sync(int argc, char *argv[])
{
	int ret, fd;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[1], O_WRONLY);
	if (fd < 0) {
		printf("open %s: %m\n", argv[1]);
		return 1;
	}

	ret = flush(fd);

	close(fd);

	return ret;
}

BAREBOX_CMD_HELP_START(sync)
BAREBOX_CMD_HELP_TEXT("Synchronize cached writes to persistent storage of DEVICE")
BAREBOX_CMD_HELP_TEXT("immediately instead of waiting for block device to be closed,")
BAREBOX_CMD_HELP_TEXT("cache eviction or the regular barebox exit.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sync)
	.cmd		= do_sync,
	BAREBOX_CMD_DESC("flush cached writes")
	BAREBOX_CMD_OPTS("DEVICE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_sync_help)
BAREBOX_CMD_END

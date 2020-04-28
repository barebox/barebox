// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* uncompress.c - uncompress a compressed file */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <uncompress.h>

static int do_uncompress(int argc, char *argv[])
{
	int from, to, ret;

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	from = open(argv[1], O_RDONLY);
	if (from < 0) {
		perror("open");
		return 1;
	}

	to = open(argv[2], O_WRONLY | O_CREAT);
	if (to < 0) {
		perror("open");
		ret = 1;
		goto exit_close;
	}

	ret = uncompress_fd_to_fd(from, to, uncompress_err_stdout);

	if (ret)
		printf("failed to decompress\n");

	close(to);
exit_close:
	close(from);
	return ret;
}


BAREBOX_CMD_START(uncompress)
	.cmd            = do_uncompress,
	BAREBOX_CMD_DESC("uncompress a compressed file")
	BAREBOX_CMD_OPTS("INFILE OUTFILE")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
BAREBOX_CMD_END

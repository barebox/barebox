/*
 * uncompress.c - uncompress a compressed file
 *
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

static const __maybe_unused char cmd_uncompress_help[] =
"Usage: uncompress <infile> <outfile>\n"
"Uncompress a compressed file\n";

BAREBOX_CMD_START(uncompress)
        .cmd            = do_uncompress,
        .usage          = "uncompress a compressed file",
        BAREBOX_CMD_HELP(cmd_uncompress_help)
BAREBOX_CMD_END


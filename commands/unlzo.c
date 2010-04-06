/*
 * unlzo.c - uncompress a lzo compressed file
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <lzo.h>

static int do_unlzo(struct command *cmdtp, int argc, char *argv[])
{
	int from, to, ret, retlen;

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

	ret = unlzo(from, to, &retlen);
	if (ret)
		printf("failed to decompress\n");

	close(to);
exit_close:
	close(from);
	return ret;
}

static const __maybe_unused char cmd_unlzo_help[] =
"Usage: unlzo <infile> <outfile>\n"
"Uncompress a lzo compressed file\n";

BAREBOX_CMD_START(unlzo)
        .cmd            = do_unlzo,
        .usage          = "lzop <infile> <outfile>",
        BAREBOX_CMD_HELP(cmd_unlzo_help)
BAREBOX_CMD_END


/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <libfile.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

extern char *mem_rw_buf;

static int do_memset(int argc, char *argv[])
{
	loff_t	s, c, n;
	int     fd;
	char   *buf;
	int	mode  = O_RWSIZE_1;
	int     ret = 1;
	char	*file = "/dev/mem";

	if (mem_parse_options(argc, argv, "bwlqd:", &mode, NULL, &file,
			NULL) < 0)
		return 1;

	if (optind + 3 > argc)
		return COMMAND_ERROR_USAGE;

	s = strtoull_suffix(argv[optind], NULL, 0);
	c = strtoull_suffix(argv[optind + 1], NULL, 0);
	n = strtoull_suffix(argv[optind + 2], NULL, 0);

	fd = open_and_lseek(file, mode | O_WRONLY | O_CREAT, s);
	if (fd < 0)
		return 1;

	buf = xmalloc(RW_BUF_SIZE);
	memset(buf, c, RW_BUF_SIZE);

	while (n > 0) {
		int now;

		now = min((loff_t)RW_BUF_SIZE, n);

		ret = write(fd, buf, now);
		if (ret < 0) {
			perror("write");
			ret = 1;
			goto out;
		}

		n -= now;
	}

	ret = 0;
out:
	close(fd);
	free(buf);

	return ret;
}

BAREBOX_CMD_HELP_START(memset)
BAREBOX_CMD_HELP_TEXT("Fills the first COUNT bytes at offset ADDR with byte DATA,")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b",  "byte access")
BAREBOX_CMD_HELP_OPT ("-w",  "word access (16 bit)")
BAREBOX_CMD_HELP_OPT ("-l",  "long access (32 bit)")
BAREBOX_CMD_HELP_OPT ("-q",  "quad access (64 bit)")
BAREBOX_CMD_HELP_OPT ("-d FILE",  "write file (default /dev/mem)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(memset)
	.cmd		= do_memset,
	BAREBOX_CMD_DESC("memory fill")
	BAREBOX_CMD_OPTS("[-bwld] ADDR DATA COUNT")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_memset_help)
BAREBOX_CMD_END

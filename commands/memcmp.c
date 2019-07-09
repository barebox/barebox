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

static int do_memcmp(int argc, char *argv[])
{
	loff_t	count;
	int     sourcefd, destfd;
	char   *buf, *source_data, *dest_data;
	int     ret = 1;
	int     offset = 0;

	if (memcpy_parse_options(argc, argv, &sourcefd, &destfd, &count,
				 O_RWSIZE_1, O_RDONLY) < 0)
		return 1;

	buf = xmalloc(RW_BUF_SIZE + RW_BUF_SIZE);
	source_data = buf;
	dest_data   = buf + RW_BUF_SIZE;

	while (count > 0) {
		int now, r1, r2, i;

		now = min((loff_t)RW_BUF_SIZE, count);

		r1 = read_full(sourcefd, source_data, now);
		if (r1 < 0) {
			perror("read");
			goto out;
		}

		r2 = read_full(destfd, dest_data, now);
		if (r2 < 0) {
			perror("read");
			goto out;
		}

		if (r1 != now || r2 != now) {
			printf("regions differ in size\n");
			goto out;
		}

		for (i = 0; i < now; i++) {
			if (source_data[i] != dest_data[i]) {
				printf("files differ at offset %d\n", offset);
				goto out;
			}
			offset++;
		}

		count -= now;
	}

	printf("OK\n");
	ret = 0;
out:
	close(sourcefd);
	close(destfd);
	free(buf);

	return ret;
}

BAREBOX_CMD_HELP_START(memcmp)
BAREBOX_CMD_HELP_TEXT("Compare memory regions specified with ADDR1 and ADDR2")
BAREBOX_CMD_HELP_TEXT("of size COUNT bytes. If source is a file, COUNT can")
BAREBOX_CMD_HELP_TEXT("be left unspecified, in which case the whole file is")
BAREBOX_CMD_HELP_TEXT("compared.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b",  "byte access")
BAREBOX_CMD_HELP_OPT ("-w",  "word access (16 bit)")
BAREBOX_CMD_HELP_OPT ("-l",  "long access (32 bit)")
BAREBOX_CMD_HELP_OPT ("-q",  "quad access (64 bit)")
BAREBOX_CMD_HELP_OPT ("-s FILE", "source file (default /dev/mem)")
BAREBOX_CMD_HELP_OPT ("-d FILE", "destination file (default /dev/mem)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(memcmp)
	.cmd		= do_memcmp,
	BAREBOX_CMD_DESC("memory compare")
	BAREBOX_CMD_OPTS("[-bwlsd] ADDR1 ADDR2 COUNT")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_memcmp_help)
BAREBOX_CMD_END

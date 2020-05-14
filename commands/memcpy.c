// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

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

static int do_memcpy(int argc, char *argv[])
{
	loff_t count;
	int sourcefd, destfd;
	int ret = 0;
	char *buf;

	if (memcpy_parse_options(argc, argv, &sourcefd, &destfd, &count,
				 0, O_WRONLY | O_CREAT) < 0)
		return 1;

	buf = xmalloc(RW_BUF_SIZE);

	while (count > 0) {
		int now, r;

		now = min_t(loff_t, RW_BUF_SIZE, count);

		r = read(sourcefd, buf, now);
		if (r < 0) {
			perror("read");
			goto out;
		}

		if (!r)
			break;

		if (write_full(destfd, buf, r) < 0) {
			perror("write");
			goto out;
		}

		count -= r;

		if (ctrlc())
			goto out;
	}

	if (count) {
		printf("ran out of data\n");
		ret = 1;
	}

out:
	free(buf);
	close(sourcefd);
	close(destfd);

	return ret;
}

BAREBOX_CMD_HELP_START(memcpy)
BAREBOX_CMD_HELP_TEXT("Copy memory at SRC of COUNT bytes to DEST")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b", "byte access")
BAREBOX_CMD_HELP_OPT ("-w", "word access (16 bit)")
BAREBOX_CMD_HELP_OPT ("-l", "long access (32 bit)")
BAREBOX_CMD_HELP_OPT ("-q", "quad access (64 bit)")
BAREBOX_CMD_HELP_OPT ("-s FILE", "source file (default /dev/mem)")
BAREBOX_CMD_HELP_OPT ("-d FILE", "write file (default /dev/mem)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(memcpy)
	.cmd		= do_memcpy,
	BAREBOX_CMD_DESC("memory copy")
	BAREBOX_CMD_OPTS("[-bwlsd] SRC DEST COUNT")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_memcpy_help)
BAREBOX_CMD_END

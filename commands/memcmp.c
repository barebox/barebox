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

static char *devmem = "/dev/mem";

static int do_memcmp(int argc, char *argv[])
{
	loff_t	addr1, addr2, count = ~0;
	int	mode  = O_RWSIZE_1;
	char   *sourcefile = devmem;
	char   *destfile = devmem;
	int     sourcefd, destfd;
	char   *rw_buf1;
	int     ret = 1;
	int     offset = 0;
	struct  stat statbuf;

	if (mem_parse_options(argc, argv, "bwlqs:d:", &mode, &sourcefile,
			&destfile, NULL) < 0)
		return 1;

	if (optind + 2 > argc)
		return COMMAND_ERROR_USAGE;

	addr1 = strtoull_suffix(argv[optind], NULL, 0);
	addr2 = strtoull_suffix(argv[optind + 1], NULL, 0);

	if (optind + 2 == argc) {
		if (sourcefile == devmem) {
			printf("source and count not given\n");
			return 1;
		}
		if (stat(sourcefile, &statbuf)) {
			perror("stat");
			return 1;
		}
		count = statbuf.st_size - addr1;
	} else {
		count = strtoull_suffix(argv[optind + 2], NULL, 0);
	}

	sourcefd = open_and_lseek(sourcefile, mode | O_RDONLY, addr1);
	if (sourcefd < 0)
		return 1;

	destfd = open_and_lseek(destfile, mode | O_RDONLY, addr2);
	if (destfd < 0) {
		close(sourcefd);
		return 1;
	}

	rw_buf1 = xmalloc(RW_BUF_SIZE);

	while (count > 0) {
		int now, r1, r2, i;

		now = min((loff_t)RW_BUF_SIZE, count);

		r1 = read_full(sourcefd, mem_rw_buf, now);
		if (r1 < 0) {
			perror("read");
			goto out;
		}

		r2 = read_full(destfd, rw_buf1, now);
		if (r2 < 0) {
			perror("read");
			goto out;
		}

		if (r1 != now || r2 != now) {
			printf("regions differ in size\n");
			goto out;
		}

		for (i = 0; i < now; i++) {
			if (mem_rw_buf[i] != rw_buf1[i]) {
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
	free(rw_buf1);

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

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
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

extern char *mem_rw_buf;

static int do_mem_md(int argc, char *argv[])
{
	loff_t	start = 0, size = 0x100;
	int	r, now;
	int	ret = 0;
	int fd;
	char *filename = "/dev/mem";
	int mode = O_RWSIZE_4;
	int swab = 0;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	if (mem_parse_options(argc, argv, "bwls:x", &mode, &filename, NULL,
			&swab) < 0)
		return 1;

	if (optind < argc) {
		if (parse_area_spec(argv[optind], &start, &size)) {
			printf("could not parse: %s\n", argv[optind]);
			return 1;
		}
		if (size == ~0)
			size = 0x100;
	}

	fd = open_and_lseek(filename, mode | O_RDONLY, start);
	if (fd < 0)
		return 1;

	do {
		now = min(size, (loff_t)RW_BUF_SIZE);
		r = read(fd, mem_rw_buf, now);
		if (r < 0) {
			perror("read");
			goto out;
		}
		if (!r)
			goto out;

		if ((ret = memory_display(mem_rw_buf, start, r,
				mode >> O_RWSIZE_SHIFT, swab)))
			goto out;

		start += r;
		size  -= r;
	} while (size);

out:
	close(fd);

	return ret ? 1 : 0;
}


BAREBOX_CMD_HELP_START(md)
BAREBOX_CMD_HELP_TEXT("Display (hex dump) a memory region.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b",  "byte access")
BAREBOX_CMD_HELP_OPT ("-w",  "word access (16 bit)")
BAREBOX_CMD_HELP_OPT ("-l",  "long access (32 bit)")
BAREBOX_CMD_HELP_OPT ("-s FILE",  "display file (default /dev/mem)")
BAREBOX_CMD_HELP_OPT ("-x",       "swap bytes at output")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Memory regions can be specified in two different forms: START+SIZE")
BAREBOX_CMD_HELP_TEXT("or START-END, If START is omitted it defaults to 0x100")
BAREBOX_CMD_HELP_TEXT("Sizes can be specified as decimal, or if prefixed with 0x as hexadecimal.")
BAREBOX_CMD_HELP_TEXT("An optional suffix of k, M or G is for kbytes, Megabytes or Gigabytes.")
BAREBOX_CMD_HELP_END


BAREBOX_CMD_START(md)
	.cmd		= do_mem_md,
	BAREBOX_CMD_DESC("memory display")
	BAREBOX_CMD_OPTS("[-bwlsx] REGION")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_md_help)
BAREBOX_CMD_END

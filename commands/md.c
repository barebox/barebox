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

static const __maybe_unused char cmd_md_help[] =
"Usage md [OPTIONS] <region>\n"
"display (hexdump) a memory region.\n"
"options:\n"
"  -s <file>   display file (default /dev/mem)\n"
"  -b          output in bytes\n"
"  -w          output in halfwords (16bit)\n"
"  -l          output in words (32bit)\n"
"  -x          swap bytes at output\n"
"\n"
"Memory regions:\n"
"Memory regions can be specified in two different forms: start+size\n"
"or start-end, If <start> is omitted it defaults to 0. If end is omitted it\n"
"defaults to the end of the device, except for interactive commands like md\n"
"and mw for which it defaults to 0x100.\n"
"Sizes can be specified as decimal, or if prefixed with 0x as hexadecimal.\n"
"an optional suffix of k, M or G is for kibibytes, Megabytes or Gigabytes,\n"
"respectively\n";


BAREBOX_CMD_START(md)
	.cmd		= do_mem_md,
	.usage		= "memory display",
	BAREBOX_CMD_HELP(cmd_md_help)
BAREBOX_CMD_END

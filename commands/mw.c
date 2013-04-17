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

static int do_mem_mw(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	char *filename = "/dev/mem";
	int mode = O_RWSIZE_4;
	loff_t adr;
	int swab = 0;

	if (mem_parse_options(argc, argv, "bwld:x", &mode, NULL, &filename,
			&swab) < 0)
		return 1;

	if (optind + 1 >= argc)
		return COMMAND_ERROR_USAGE;

	adr = strtoull_suffix(argv[optind++], NULL, 0);

	fd = open_and_lseek(filename, mode | O_WRONLY, adr);
	if (fd < 0)
		return 1;

	while (optind < argc) {
		u8 val8;
		u16 val16;
		u32 val32;
		switch (mode) {
		case O_RWSIZE_1:
			val8 = simple_strtoul(argv[optind], NULL, 0);
			ret = write(fd, &val8, 1);
			break;
		case O_RWSIZE_2:
			val16 = simple_strtoul(argv[optind], NULL, 0);
			if (swab)
				val16 = __swab16(val16);
			ret = write(fd, &val16, 2);
			break;
		case O_RWSIZE_4:
			val32 = simple_strtoul(argv[optind], NULL, 0);
			if (swab)
				val32 = __swab32(val32);
			ret = write(fd, &val32, 4);
			break;
		}
		if (ret < 0) {
			perror("write");
			break;
		}
		ret = 0;
		optind++;
	}

	close(fd);

	return ret ? 1 : 0;
}

static const __maybe_unused char cmd_mw_help[] =
"Usage: mw [OPTIONS] <region> <value(s)>\n"
"Write value(s) to the specifies region.\n"
"options:\n"
"  -b, -w, -l	use byte, halfword, or word accesses\n"
"  -d <file>	write file (default /dev/mem)\n";

BAREBOX_CMD_START(mw)
	.cmd		= do_mem_mw,
	.usage		= "memory write (fill)",
	BAREBOX_CMD_HELP(cmd_mw_help)
BAREBOX_CMD_END

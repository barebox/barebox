/*
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

static int do_mem_mm(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	char *filename = "/dev/mem";
	int mode = O_RWSIZE_4;
	loff_t adr;
	int swab = 0;
	u8 val8;
	u16 val16;
	u32 val32;
	u32 value, mask;

	if (mem_parse_options(argc, argv, "bwld:", &mode, NULL, &filename,
			&swab) < 0)
		return 1;

	if (optind + 2 >= argc)
		return COMMAND_ERROR_USAGE;

	adr = strtoull_suffix(argv[optind++], NULL, 0);
	value = simple_strtoul(argv[optind++], NULL, 0);
	mask = simple_strtoul(argv[optind++], NULL, 0);

	fd = open_and_lseek(filename, mode | O_RDWR, adr);
	if (fd < 0)
		return 1;

	switch (mode) {
	case O_RWSIZE_1:
		ret = pread(fd, &val8, 1, adr);
		if (ret < 0)
			goto out_read;
		val8 &= ~mask;
		val8 |= (value & mask);
		ret = pwrite(fd, &val8, 1, adr);
		if (ret < 0)
			goto out_write;
		break;
	case O_RWSIZE_2:
		ret = pread(fd, &val16, 2, adr);
		if (ret < 0)
			goto out_read;
		val16 &= ~mask;
		val16 |= (value & mask);
		ret = pwrite(fd, &val16, 2, adr);
		if (ret < 0)
			goto out_write;
		break;
	case O_RWSIZE_4:
		if (ret < 0)
			goto out_read;
		ret = pread(fd, &val32, 4, adr);
		val32 &= ~mask;
		val32 |= (value & mask);
		ret = pwrite(fd, &val32, 4, adr);
		if (ret < 0)
			goto out_write;
		break;
	}

	close(fd);

	return 0;

out_read:
	perror("read");
	close(fd);
	return 1;

out_write:
	perror("write");
	close(fd);
	return 1;
}

static const __maybe_unused char cmd_mm_help[] =
"Usage: mm [OPTIONS] <adr> <val> <mask>\n"
"set/clear bits specified with <mask> in <adr> to <value>\n"
"options:\n"
"  -b, -w, -l	use byte, halfword, or word accesses\n"
"  -d <file>	write file (default /dev/mem)\n";

BAREBOX_CMD_START(mm)
	.cmd		= do_mem_mm,
	.usage		= "memory modify write with mask",
	BAREBOX_CMD_HELP(cmd_mm_help)
BAREBOX_CMD_END

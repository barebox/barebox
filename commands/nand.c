/*
 * Copyright (c) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <xfuncs.h>
#include <init.h>
#include <ioctl.h>
#include <nand.h>
#include <linux/mtd/mtd-abi.h>
#include <fcntl.h>
#include <libgen.h>

#define NAND_ADD	1
#define NAND_DEL	2
#define NAND_MARKBAD	3

static int do_nand(int argc, char *argv[])
{
	int opt;
	int command = 0;
	loff_t badblock = 0;

	while((opt = getopt(argc, argv, "adb:")) > 0) {
		if (command) {
			printf("only one command may be given\n");
			return 1;
		}

		switch (opt) {
		case 'a':
			command = NAND_ADD;
			break;
		case 'd':
			command = NAND_DEL;
			break;
		case 'b':
			command = NAND_MARKBAD;
			badblock = strtoull_suffix(optarg, NULL, 0);
		}
	}

	if (optind >= argc)
		return COMMAND_ERROR_USAGE;

	if (command == NAND_ADD) {
		while (optind < argc) {
			if (dev_add_bb_dev(basename(argv[optind]), NULL))
				return 1;

			optind++;
		}
	}

	if (command == NAND_DEL) {
		while (optind < argc) {
			dev_remove_bb_dev(basename(argv[optind]));
			optind++;
		}
	}

	if (command == NAND_MARKBAD) {
		int ret = 0, fd;

		printf("marking block at 0x%08llx on %s as bad\n",
				badblock, argv[optind]);

		fd = open(argv[optind], O_RDWR);
		if (fd < 0) {
			perror("open");
			return 1;
		}

		ret = ioctl(fd, MEMSETBADBLOCK, &badblock);
		if (ret)
			perror("ioctl");

		close(fd);
		return ret;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(nand)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a",  "register a bad block aware device ontop of a normal NAND device")
BAREBOX_CMD_HELP_OPT ("-d",  "deregister a bad block aware device")
BAREBOX_CMD_HELP_OPT ("-b OFFS",  "mark block at OFFSet as bad")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(nand)
	.cmd		= do_nand,
	BAREBOX_CMD_DESC("NAND flash handling")
	BAREBOX_CMD_OPTS("[-adb] NANDDEV")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_nand_help)
BAREBOX_CMD_END

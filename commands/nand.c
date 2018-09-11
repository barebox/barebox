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
#define NAND_MARKGOOD	4
#define NAND_INFO	5

static int do_nand(int argc, char *argv[])
{
	int opt;
	int command = 0;
	loff_t badblock = 0;
	int fd;
	int ret = 0;
	struct mtd_info_user mtdinfo;

	while((opt = getopt(argc, argv, "adb:g:i")) > 0) {
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
			break;
		case 'g':
			command = NAND_MARKGOOD;
			badblock = strtoull_suffix(optarg, NULL, 0);
			break;
		case 'i':
			command = NAND_INFO;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind >= argc)
		return COMMAND_ERROR_USAGE;

	if (!command) {
		printf("No action given\n");
		return COMMAND_ERROR_USAGE;
	}

	if (command == NAND_ADD) {
		while (optind < argc) {
			if (dev_add_bb_dev(basename(argv[optind]), NULL))
				return 1;

			optind++;
		}

		goto out_ret;
	}

	if (command == NAND_DEL) {
		while (optind < argc) {
			if (dev_remove_bb_dev(basename(argv[optind])))
				return 1;
			optind++;
		}

		goto out_ret;
	}

	fd = open(argv[optind], O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = ioctl(fd, MEMGETINFO, &mtdinfo);
	if (ret)
		goto out;

	if (command == NAND_MARKBAD || command == NAND_MARKGOOD) {
		const char *str;
		int ctl;

		if (command == NAND_MARKBAD) {
			str = "bad";
			ctl = MEMSETBADBLOCK;
		} else {
			str = "good";
			ctl = MEMSETGOODBLOCK;
		}

		printf("marking block at 0x%08llx on %s as %s\n",
				badblock, argv[optind], str);

		ret = ioctl(fd, ctl, &badblock);
		if (ret) {
			if (ret == -EINVAL)
				printf("Maybe offset %lld is out of range.\n",
					badblock);
			else
				perror("ioctl");
		}

		goto out;
	}

	if (command == NAND_INFO) {
		loff_t ofs;
		int bad = 0;

		for (ofs = 0; ofs < mtdinfo.size; ofs += mtdinfo.erasesize) {
			if (ioctl(fd, MEMGETBADBLOCK, &ofs)) {
				printf("Block at 0x%08llx is bad\n", ofs);
				bad = 1;
			}
		}

		if (!bad)
			printf("No bad blocks\n");
	}

out:
	close(fd);

out_ret:
	return ret;
}

BAREBOX_CMD_HELP_START(nand)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a",  "register a bad block aware device ontop of a normal NAND device")
BAREBOX_CMD_HELP_OPT ("-d",  "deregister a bad block aware device")
BAREBOX_CMD_HELP_OPT ("-b OFFS",  "mark block at OFFSet as bad")
BAREBOX_CMD_HELP_OPT ("-g OFFS",  "mark block at OFFSet as good")
BAREBOX_CMD_HELP_OPT ("-i",  "info. Show information about bad blocks")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(nand)
	.cmd		= do_nand,
	BAREBOX_CMD_DESC("NAND flash handling")
	BAREBOX_CMD_OPTS("[-adb] NANDDEV")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_nand_help)
BAREBOX_CMD_END

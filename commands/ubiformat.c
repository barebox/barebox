/*
 * Copyright (C) 2008 Nokia Corporation
 * Copyright (C) 2012 Wolfram Sang, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 */

/*
 * An utility to format MTD devices into UBI and flash UBI images.
 *
 * Author: Artem Bityutskiy
 */

/*
 * Maximum amount of consequtive eraseblocks which are considered as normal by
 * this utility. Otherwise it is assume that something is wrong with the flash
 * or the driver, and eraseblocks are stopped being marked as bad.
 */
#define MAX_CONSECUTIVE_BAD_BLOCKS 4

#define PROGRAM_NAME	"ubiformat"

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <crc.h>
#include <stdlib.h>
#include <clock.h>
#include <malloc.h>
#include <ioctl.h>
#include <libbb.h>
#include <libfile.h>
#include <ubiformat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/ubi.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/log2.h>
#include <linux/mtd/mtd-abi.h>
#include <mtd/libscan.h>
#include <mtd/libubigen.h>
#include <mtd/ubi-user.h>
#include <mtd/utils.h>
#include <mtd/ubi-media.h>
#include <mtd/mtd-peb.h>

static int parse_opt(int argc, char *argv[], struct ubiformat_args *args,
		     char **node)
{
	srand(get_time_ns());
	memset(args, 0, sizeof(*args));

	while (1) {
		int key;
		unsigned long int image_seq;

		key = getopt(argc, argv, "nyqve:x:s:O:f:S:Q:");
		if (key == -1)
			break;

		switch (key) {
		case 's':
			args->subpage_size = strtoull_suffix(optarg, NULL, 0);
			if (args->subpage_size <= 0)
				return errmsg("bad sub-page size: \"%s\"", optarg);
			if (!is_power_of_2(args->subpage_size))
				return errmsg("sub-page size should be power of 2");
			break;

		case 'O':
			args->vid_hdr_offs = simple_strtoul(optarg, NULL, 0);
			if (args->vid_hdr_offs <= 0)
				return errmsg("bad VID header offset: \"%s\"", optarg);
			break;

		case 'e':
			args->ec = simple_strtoull(optarg, NULL, 0);
			if (args->ec < 0)
				return errmsg("bad erase counter value: \"%s\"", optarg);
			if (args->ec >= EC_MAX)
				return errmsg("too high erase %llu, counter, max is %u", args->ec, EC_MAX);
			args->override_ec = 1;
			break;

		case 'f':
			args->image = optarg;
			break;

		case 'n':
			args->novtbl = 1;
			break;

		case 'y':
			args->yes = 1;
			break;

		case 'q':
			args->quiet = 1;
			break;

		case 'x':
			args->ubi_ver = simple_strtoul(optarg, NULL, 0);
			if (args->ubi_ver < 0)
				return errmsg("bad UBI version: \"%s\"", optarg);
			break;

		case 'Q':
			image_seq = simple_strtoul(optarg, NULL, 0);
			if (image_seq > 0xFFFFFFFF)
				return errmsg("bad UBI image sequence number: \"%s\"", optarg);
			args->image_seq = image_seq;
			break;

		case 'v':
			args->verbose = 1;
			break;

		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (args->quiet && args->verbose)
		return errmsg("using \"-q\" and \"-v\" at the same time does not make sense");

	if (optind == argc)
		return errmsg("MTD device name was not specified");
	else if (optind != argc - 1)
		return errmsg("more then one MTD device specified");

	if (args->image && args->novtbl)
		return errmsg("-n cannot be used together with -f");


	*node = argv[optind];

	return 0;
}

static int do_ubiformat(int argc, char *argv[])
{
	int err, fd;
	struct ubiformat_args args;
	char *node = NULL;
	struct mtd_info_user mtd_user;

	err = parse_opt(argc, argv, &args, &node);
	if (err)
		return err;

	fd = open(node, O_RDWR);
	if (fd < 0)
		return sys_errmsg("cannot open \"%s\"", node);

	err = ioctl(fd, MEMGETINFO, &mtd_user);
	if (err) {
		sys_errmsg("MEMGETINFO ioctl request failed");
		goto out_close;
	}

	err = ubiformat(mtd_user.mtd, &args);

out_close:
	close(fd);

	return err;
}

BAREBOX_CMD_HELP_START(ubiformat)
BAREBOX_CMD_HELP_TEXT("A tool to format MTD devices and flash UBI images")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-s BYTES", "minimum input/output unit used for UBI headers")
BAREBOX_CMD_HELP_OPT("\t", "e.g. sub-page size in case of NAND flash")
BAREBOX_CMD_HELP_OPT("-O OFFS\t", "offset if the VID header from start of the")
BAREBOX_CMD_HELP_OPT("\t", "physical eraseblock (default is the next minimum I/O unit or")
BAREBOX_CMD_HELP_OPT("\t", "sub-page after the EC header)")
BAREBOX_CMD_HELP_OPT("-n\t", "only erase all eraseblock and preserve erase")
BAREBOX_CMD_HELP_OPT("\t", "counters, do not write empty volume table")
BAREBOX_CMD_HELP_OPT("-f FILE\t", "flash image file")
BAREBOX_CMD_HELP_OPT("-e VALUE", "use VALUE as erase counter value for all eraseblocks")
BAREBOX_CMD_HELP_OPT("-x NUM\t", "UBI version number to put to EC headers (default 1)")
BAREBOX_CMD_HELP_OPT("-Q NUM\t", "32-bit UBI image sequence number to use")
BAREBOX_CMD_HELP_OPT("-y\t", "Assume yes for all questions")
BAREBOX_CMD_HELP_OPT("-q\t", "suppress progress percentage information")
BAREBOX_CMD_HELP_OPT("-v\t", "be verbose")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Example 1: ubiformat /dev/nand0 -y - format nand0 and assume yes")
BAREBOX_CMD_HELP_TEXT("Example 2: ubiformat /dev/nand0 -q -e 0 - format nand0,")
BAREBOX_CMD_HELP_TEXT("\tbe quiet and force erase counter value 0.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ubiformat)
	.cmd		= do_ubiformat,
	BAREBOX_CMD_DESC("format an ubi volume")
	BAREBOX_CMD_OPTS("[-sOnfexQqv] MTDEVICE")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_ubiformat_help)
BAREBOX_CMD_END

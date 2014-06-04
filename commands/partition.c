/*
 * partition.c - parse a linux-like mtd partition definition and register
 *               the new partitions
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

/**
 * @file
 * @brief partition handling and addpart and delpart command
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <driver.h>
#include <malloc.h>
#include <partition.h>
#include <errno.h>
#include <xfuncs.h>
#include <fs.h>
#include <linux/stat.h>
#include <libgen.h>
#include <getopt.h>
#include <linux/err.h>

#define SIZE_REMAINING ((ulong)-1)

#define PART_ADD_DEVNAME (1 << 0)

static int mtd_part_do_parse_one(char *devname, const char *partstr,
				 char **endp, loff_t *offset,
				 loff_t devsize, size_t *retsize,
				 unsigned int pflags)
{
	loff_t size;
	char *end;
	char buf[PATH_MAX] = {};
	unsigned long flags = 0;
	int ret = 0;
	struct cdev *cdev;

	memset(buf, 0, PATH_MAX);

	if (*partstr == '-') {
		size = SIZE_REMAINING;
		end = (char *)partstr + 1;
	} else {
		size = strtoull_suffix(partstr, &end, 0);
	}

	if (*end == '@')
		*offset = strtoull_suffix(end+1, &end, 0);

	if (size == SIZE_REMAINING)
		size = devsize - *offset;

	partstr = end;

	if (*partstr == '(') {
		partstr++;
		end = strchr((char *) partstr, ')');
		if (!end) {
			printf("could not find matching ')'\n");
			return -EINVAL;
		}

		if (pflags & PART_ADD_DEVNAME)
			sprintf(buf, "%s.", devname);
		memcpy(buf + strlen(buf), partstr, end - partstr);

		end++;
	}

	if (size + *offset > devsize) {
		printf("%s: partition end is beyond device\n", buf);
		return -EINVAL;
	}

	partstr = end;

	if (*partstr == 'r' && *(partstr + 1) == 'o') {
		flags |= DEVFS_PARTITION_READONLY;
		end = (char *)(partstr + 2);
	}

	if (endp)
		*endp = end;

	*retsize = size;

	cdev = devfs_add_partition(devname, *offset, size, flags, buf);
	if (IS_ERR(cdev)) {
		ret = PTR_ERR(cdev);
		printf("cannot create %s: %s\n", buf, strerror(-ret));
	}

	return ret;
}

static int do_addpart(int argc, char *argv[])
{
	char *devname;
	char *endp;
	loff_t offset = 0;
	loff_t devsize;
	struct stat s;
	int opt;
	unsigned int flags = PART_ADD_DEVNAME;

	while ((opt = getopt(argc, argv, "n")) > 0) {
		switch (opt) {
		case 'n':
			flags &= ~PART_ADD_DEVNAME;
			break;
		}
	}

	if (argc != optind + 2)
		return COMMAND_ERROR_USAGE;

	if (stat(argv[optind], &s)) {
		perror("addpart");
		return 1;
	}
	devsize = s.st_size;

	devname = basename(argv[optind]);

	endp = argv[optind + 1];

	while (1) {
		size_t size = 0;

		if (mtd_part_do_parse_one(devname, endp, &endp, &offset,
					devsize, &size, flags))
			return 1;

		offset += size;

		if (!*endp)
			break;

		if (*endp != ',') {
			printf("parse error\n");
			return 1;
		}
		endp++;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(addpart)

BAREBOX_CMD_HELP_TEXT("The size and the offset can be given in decimal (without any prefix) and")
BAREBOX_CMD_HELP_TEXT("in hex (prefixed with 0x). Both can have an optional suffix K, M or G.")
BAREBOX_CMD_HELP_TEXT("The size of the last partition can be specified as '-' for the remaining")
BAREBOX_CMD_HELP_TEXT("space on the device.  This format is the same as used by the Linux")
BAREBOX_CMD_HELP_TEXT("kernel or cmdline mtd partitions.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-n", "do not use the device name as prefix of the partition name")
BAREBOX_CMD_HELP_OPT ("DEVICE",    "device being worked on")
BAREBOX_CMD_HELP_OPT ("PART", "SIZE1[@OFFSET1](NAME1)[RO],SIZE2[@OFFSET2](NAME2)[RO],...")
BAREBOX_CMD_HELP_END

/**
 * @page addpart_command

The size and the offset can be given in decimal (without any prefix) and
in hex (prefixed with 0x). Both can have an optional suffix K, M or G.
The size of the last partition can be specified as '-' for the remaining
space on the device.  This format is the same as used by the Linux
kernel or cmdline mtd partitions.

\todo This command has to be reworked and will probably change it's API.
*/

BAREBOX_CMD_START(addpart)
	.cmd = do_addpart,
	BAREBOX_CMD_DESC("add a partition description to a device")
	BAREBOX_CMD_OPTS("[-n] DEVICE PART")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_addpart_help)
BAREBOX_CMD_END

static int do_delpart(int argc, char *argv[])
{
	int i, err;

	for (i = 1; i < argc; i++) {
		err = devfs_del_partition(basename(argv[i]));
		if (err) {
			printf("cannot delete %s: %s\n", argv[i], strerror(-err));
			break;
		}
	}

	return 1;
}

BAREBOX_CMD_HELP_START(delpart)
BAREBOX_CMD_HELP_TEXT("Delete partitions previously added to a device with addpart.")
BAREBOX_CMD_HELP_END

/**
 * @page delpart_command

Partitions are created by adding their description with the addpart
command. If you want to get rid of a partition again, use delpart. The
argument list is taken as a list of partitions to be deleted.

\todo Add an example

 */

BAREBOX_CMD_START(delpart)
	.cmd = do_delpart,
	BAREBOX_CMD_DESC("delete partition(s)")
	BAREBOX_CMD_OPTS("PART...")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_delpart_help)
	BAREBOX_CMD_COMPLETE(devfs_partition_complete)
BAREBOX_CMD_END


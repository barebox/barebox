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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file
 * @brief partition handling and addpart and delpart command
 */

#ifdef CONFIG_ENABLE_PARTITION_NOISE
# define DEBUG
#endif

#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <partition.h>
#include <errno.h>
#include <xfuncs.h>
#include <fs.h>
#include <linux/stat.h>
#include <libgen.h>

static int mtd_part_do_parse_one(char *devname, const char *partstr,
				 char **endp, unsigned long offset,
				 off_t devsize, size_t *retsize)
{
	ulong size;
	char *end;
	char buf[PATH_MAX];
	unsigned long flags = 0;
	int ret;

	memset(buf, 0, PATH_MAX);

	if (*partstr == '-') {
		size = devsize - offset;
		end = (char *)partstr + 1;
	} else {
		size = strtoul_suffix(partstr, &end, 0);
	}

	partstr = end;

	if (*partstr == '(') {
		partstr++;
		end = strchr((char *) partstr, ')');
		if (!end) {
			printf("could not find matching ')'\n");
			return -EINVAL;
		}

		sprintf(buf, "%s.", devname);
		memcpy(buf + strlen(buf), partstr, end - partstr);

		end++;
	}

	if (size + offset > devsize) {
		printf("%s: partition end is beyond device\n", buf);
		return -EINVAL;
	}

	partstr = end;

	if (*partstr == 'r' && *(partstr + 1) == 'o') {
		flags |= PARTITION_READONLY;
		end = (char *)(partstr + 2);
	}

	if (endp)
		*endp = end;

	*retsize = size;

	ret = devfs_add_partition(devname, offset, size, flags, buf);
	if (ret)
		printf("cannot create %s: %s\n", buf, strerror(-ret));
	return ret;
}

static int do_addpart(cmd_tbl_t * cmdtp, int argc, char *argv[])
{
	char *devname;
	char *endp;
	unsigned long offset = 0;
	off_t devsize;
	struct stat s;

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	if (stat(argv[1], &s)) {
		perror("addpart");
		return 1;
	}
	devsize = s.st_size;

	devname = basename(argv[1]);

	endp = argv[2];

	while (1) {
		size_t size = 0;

		if (mtd_part_do_parse_one(devname, endp, &endp, offset, devsize, &size))
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

static const __maybe_unused char cmd_addpart_help[] =
"Usage: addpart <device> <partition description>\n"
"\n"
"addpart adds a partition description to a device. The partition description\n"
"has the form\n"
"size1(name1)[ro],size2(name2)[ro],...\n"
"<device> is the device name under. Size can be given in decimal or if prefixed\n"
"with 0x in hex. Sizes can have an optional suffix K,M,G. The size of the last\n"
"partition can be specified as '-' for the remaining space of the device.\n"
"This format is the same as used in the Linux kernel for cmdline mtd partitions.\n"
"\n"
"Note: That this command has to be reworked and will probably change it's API.";

BAREBOX_CMD_START(addpart)
	.cmd = do_addpart,
	.usage = "adds a partition table to a device",
	BAREBOX_CMD_HELP(cmd_addpart_help)
BAREBOX_CMD_END

/** @page addpart_command addpart Add a partition to a device
 *
 * Usage is: addpart \<device> \<partition description>
 *
 * Adds a partition description to a device. The partition description has the
 * form
 *
 * size1(name1)[ro],size2(name2)[ro],...
 *
 * \<device> is the device name under. Sizes can be given in decimal or - if
 * prefixed with 0x - in hex. Sizes can have an optional suffix K,M,G. The
 * size of the last partition can be specified as '-' for the remaining space
 * of the device.
 *
 * @note The format is the same as used in the Linux kernel for cmdline mtd
 * partitions.
 *
 * @note This command has to be reworked and will probably change it's API.
 */

static int do_delpart(cmd_tbl_t * cmdtp, int argc, char *argv[])
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

static const __maybe_unused char cmd_delpart_help[] =
"Usage: delpart FILE...\n"
"Delete partitions previously added to a device with addpart.\n";

BAREBOX_CMD_START(delpart)
	.cmd = do_delpart,
	.usage = "delete partition(s)",
	BAREBOX_CMD_HELP(cmd_delpart_help)
BAREBOX_CMD_END

/** @page delpart_command delpart Delete a partition
 *
 * Usage is: delpart \<partions>
 *
 * Delete a partition previously added to a device with addpart.
 */

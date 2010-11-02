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

#define SIZE_REMAINING ((ulong)-1)

static int mtd_part_do_parse_one(char *devname, const char *partstr,
				 char **endp, unsigned long *offset,
				 off_t devsize, size_t *retsize)
{
	ulong size;
	char *end;
	char buf[PATH_MAX];
	unsigned long flags = 0;
	int ret;

	memset(buf, 0, PATH_MAX);

	if (*partstr == '-') {
		size = SIZE_REMAINING;
		end = (char *)partstr + 1;
	} else {
		size = strtoul_suffix(partstr, &end, 0);
	}

	if (*end == '@')
		*offset = strtoul_suffix(end+1, &end, 0);

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
		flags |= PARTITION_READONLY;
		end = (char *)(partstr + 2);
	}

	if (endp)
		*endp = end;

	*retsize = size;

	ret = devfs_add_partition(devname, *offset, size, flags, buf);
	if (ret)
		printf("cannot create %s: %s\n", buf, strerror(-ret));
	return ret;
}

static int do_addpart(struct command * cmdtp, int argc, char *argv[])
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

		if (mtd_part_do_parse_one(devname, endp, &endp, &offset, devsize, &size))
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
BAREBOX_CMD_HELP_USAGE("addpart <device> <part_desc>\n")
BAREBOX_CMD_HELP_SHORT("Add a partition description to a device.\n")
BAREBOX_CMD_HELP_OPT  ("<device>",    "device being worked on\n")
BAREBOX_CMD_HELP_OPT  ("<part_desc>", "size1[@offset1](name1)[ro],size2[@offset2](name2)[ro],...\n")
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
	.usage = "adds a partition table to a device",
	BAREBOX_CMD_HELP(cmd_addpart_help)
BAREBOX_CMD_END

static int do_delpart(struct command * cmdtp, int argc, char *argv[])
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
BAREBOX_CMD_HELP_USAGE("delpart <part 1> [<part n>] \n")
BAREBOX_CMD_HELP_SHORT("Delete partitions previously added to a device with addpart.\n")
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
	.usage = "delete partition(s)",
	BAREBOX_CMD_HELP(cmd_delpart_help)
BAREBOX_CMD_END


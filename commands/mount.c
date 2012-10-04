/*
 * mount.c - mount devices
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
 * @brief Filesystem mounting support
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>

static int do_mount(int argc, char *argv[])
{
	int opt;
	int ret = 0;
	struct fs_device_d *fsdev;
	char *type = NULL;

	if (argc == 1) {
		for_each_fs_device(fsdev) {
			printf("%s on %s type %s\n",
				fsdev->parent_device ? dev_name(fsdev->parent_device) : "none",
				fsdev->path,
				fsdev->dev.name);
		}
		return 0;
	}

	while ((opt = getopt(argc, argv, "t:")) > 0) {
		switch (opt) {
		case 't':
			type = optarg;
			break;
		}
	}

	if (argc < optind + 2)
		return COMMAND_ERROR_USAGE;

	if ((ret = mount(argv[optind], type, argv[optind + 1]))) {
		perror("mount");
		return 1;
	}
	return 0;
}

BAREBOX_CMD_HELP_START(mount)
BAREBOX_CMD_HELP_USAGE("mount [[-t <fstype] <device> <mountpoint>]\n")
BAREBOX_CMD_HELP_SHORT("Mount a filesystem of a given type to a mountpoint.\n")
BAREBOX_CMD_HELP_SHORT("If no fstype is specified, try to detect it automatically.\n")
BAREBOX_CMD_HELP_SHORT("If no argument is given, list mounted filesystems.\n")
BAREBOX_CMD_HELP_END

/**
 * @page mount_command

<ul>
<li>\<device> can be a device in /dev or some arbitrary string if no
    device is needed for this driver, i.e. on ramfs. </li>
<li>\<fstype> is the filesystem driver. A list of available drivers can
    be shown with the \ref devinfo_command command.</li>
<li>\<mountpoint> must be an empty directory, one level below the /
    directory.</li>
</ul>

 */

/**
 * @page how_mount_works How mount works in barebox

Mounting a filesystem ontop of a device is working like devices and
drivers are finding together.

The mount command creates a new device with the filesystem name as the
driver for this "device". So the framework is able to merge both parts
together.

By the way: With this feature its impossible to accidentely remove
partitions in use. A partition is internally also a device. If its
mounted it will be marked as busy, so an delpart command fails, until
the filesystem has been unmounted.

 */

BAREBOX_CMD_START(mount)
	.cmd		= do_mount,
	.usage		= "Mount a filesystem of a given type to a mountpoint or list mounted filesystems.",
	BAREBOX_CMD_HELP(cmd_mount_help)
BAREBOX_CMD_END

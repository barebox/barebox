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

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>
#include <linux/err.h>

static int do_mount(int argc, char *argv[])
{
	int opt, verbose = 0;
	struct driver_d *drv;
	const char *type = NULL;
	const char *mountpoint, *devstr;
	const char *fsoptions = NULL;

	while ((opt = getopt(argc, argv, "ao:t:v")) > 0) {
		switch (opt) {
		case 'a':
			mount_all();
			break;
		case 't':
			type = optarg;
			break;
		case 'o':
			fsoptions = optarg;
			break;
		case 'v':
			verbose++;
			break;
		}
	}

	if (argc == optind) {
		struct fs_device_d *fsdev;

		for_each_fs_device(fsdev) {
			printf("%s on %s type %s\n",
				fsdev->backingstore ? fsdev->backingstore : "none",
				fsdev->path,
				fsdev->dev.name);
		}

		if (verbose) {
			printf("\nSupported filesystems:\n\n");
			bus_for_each_driver(&fs_bus, drv) {
				struct fs_driver_d * fsdrv = drv_to_fs_driver(drv);
				printf("%s\n", fsdrv->drv.name);
			}
		}

		return 0;
	}

	devstr = argv[optind];

	if (argc == optind + 1) {
		struct cdev *cdev;
		const char *path;

		device_detect_by_name(devpath_to_name(devstr));

		cdev = cdev_by_name(devstr);
		if (!cdev)
			return -ENOENT;

		path = cdev_mount_default(cdev, fsoptions);
		if (IS_ERR(path))
			return PTR_ERR(path);

		printf("mounted /dev/%s on %s\n", devstr, path);

		return 0;
	}

	if (argc < optind + 2)
		return COMMAND_ERROR_USAGE;

	if (argc == optind + 3) {
		/*
		 * Old behaviour: mount <dev> <type> <mountpoint>
		 */
		type = argv[optind + 1];
		mountpoint = argv[optind + 2];
	} else {
		mountpoint = argv[optind + 1];
	}

	return mount(devstr, type, mountpoint, fsoptions);
}

BAREBOX_CMD_HELP_START(mount)
BAREBOX_CMD_HELP_TEXT("If no argument is given, list mounted filesystems.")
BAREBOX_CMD_HELP_TEXT("If no FSTYPE is specified, try to detect it automatically.")
BAREBOX_CMD_HELP_TEXT("With -a the mount command mounts all block devices whose filesystem")
BAREBOX_CMD_HELP_TEXT("can be detected automatically to /mnt/PARTNAME")
BAREBOX_CMD_HELP_TEXT("If mountpoint is not given, a standard mountpoint of /mnt/DEVICE")
BAREBOX_CMD_HELP_TEXT("is used. This directoy is created automatically if necessary.")
BAREBOX_CMD_HELP_TEXT("With -o loop the mount command mounts a file instead of a device.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-a\t", "mount all blockdevices")
BAREBOX_CMD_HELP_OPT("-t FSTYPE", "specify filesystem type")
BAREBOX_CMD_HELP_OPT("-o OPTIONS", "set file system OPTIONS")
BAREBOX_CMD_HELP_OPT("-v\t", "verbose")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mount)
	.cmd		= do_mount,
	BAREBOX_CMD_DESC("mount a filesystem or list mounted filesystems")
	BAREBOX_CMD_OPTS("[[-atov] [DEVICE] [MOUNTPOINT]]")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_mount_help)
BAREBOX_CMD_END

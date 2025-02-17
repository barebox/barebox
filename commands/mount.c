// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* mount.c - mount devices */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>
#include <linux/err.h>
#include <stringlist.h>

static int do_mount(int argc, char *argv[])
{
	int ret = 0, opt, verbose = 0;
	struct driver *drv;
	const char *type = NULL;
	const char *mountpoint, *devstr;
	struct string_list fsoption_list;
	char *fsoptions = NULL;

	string_list_init(&fsoption_list);

	while ((opt = getopt(argc, argv, "ao:t:v")) > 0) {
		switch (opt) {
		case 'a':
			mount_all();
			break;
		case 't':
			type = optarg;
			break;
		case 'o':
			string_list_add(&fsoption_list, optarg);
			break;
		case 'v':
			verbose++;
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc == 0) {
		struct fs_device *fsdev;

		for_each_fs_device(fsdev) {
			printf("%s on %s type %s\n",
				fsdev->backingstore ? fsdev->backingstore : "none",
				fsdev->path,
				fsdev->dev.name);
		}

		if (verbose) {
			printf("\nSupported filesystems:\n\n");
			bus_for_each_driver(&fs_bus, drv) {
				struct fs_driver * fsdrv = drv_to_fs_driver(drv);
				printf("%s\n", fsdrv->drv.name);
			}
		}

		goto out;
	}

	if (argc == 0) {
		ret = COMMAND_ERROR_USAGE;
		goto out;
	}

	devstr = argv[0];

	fsoptions = string_list_join(&fsoption_list, " ");
	if (!fsoptions) {
		ret = -ENOMEM;
		goto out;
	}

	if (argc == 1) {
		struct cdev *cdev;
		const char *path;

		devstr = devpath_to_name(devstr);

		device_detect_by_name(devstr);

		cdev = cdev_by_name(devstr);
		if (!cdev) {
			ret = -ENOENT;
			goto out;
		}

		path = cdev_mount_default(cdev, fsoptions);
		if (IS_ERR(path))
			return PTR_ERR(path);

		printf("mounted /dev/%s on %s\n", devstr, path);

		goto out;
	}

	if (argc == 3) {
		/*
		 * Old behaviour: mount <dev> <type> <mountpoint>
		 */
		type = argv[1];
		mountpoint = argv[2];
	} else {
		mountpoint = argv[1];
	}

	ret = mount(devstr, type, mountpoint, fsoptions);

out:
	free(fsoptions);
	string_list_free(&fsoption_list);
	return ret;
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

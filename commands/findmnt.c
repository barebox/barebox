// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2022 Ahmad Fatoum, Pengutronix

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>

static void print_header(bool *header_printed)
{
	if (*header_printed)
		return;
	printf("%-20s%-25s%-10s%-20s\n", "TARGET", "SOURCE", "FSTYPE", "OPTIONS");
	*header_printed = true;
}

static void report_findmnt(const struct fs_device *fsdev)
{
	const char *backingstore;

	backingstore = fsdev->backingstore ?: cdev_name(fsdev->cdev) ?: "none";

	printf("%-20s%-25s%-10s%-20s\n", fsdev->path, backingstore,
	       fsdev->driver->drv.name, fsdev->options);
}

static int do_findmnt(int argc, char *argv[])
{
	bool header_printed = false;
	struct fs_device *target = NULL;
	char *device = NULL;
	int opt, dirfd = AT_FDCWD;

	while ((opt = getopt(argc, argv, "T:")) > 0) {
		switch(opt) {
		case 'T':
			target = get_fsdevice_by_path(dirfd, optarg);
			if (!target)
				return COMMAND_ERROR;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if ((target && argc > 0) || (!target && argc > 1))
		return COMMAND_ERROR_USAGE;

	if (target) {
		print_header(&header_printed);
		report_findmnt(target);
		return 0;
	}

	if (argv[0]) {
		device = canonicalize_path(dirfd, argv[0]);
		if (!device)
			return COMMAND_ERROR;
	}

	for_each_fs_device(target) {
		if (!device || streq_ptr(target->path, device) ||
		    streq_ptr(target->backingstore, device)) {
			print_header(&header_printed);
			report_findmnt(target);
		} else {
			const char *backingstore;
			struct cdev *cdev;

			cdev = cdev_open_by_name(devpath_to_name(device), O_RDONLY);
			if (!cdev)
				continue;

			backingstore = target->backingstore;
			backingstore += str_has_prefix(backingstore, "/dev/");

			if (streq_ptr(backingstore, cdev->name)) {
				print_header(&header_printed);
				report_findmnt(target);
			}
			cdev_close(cdev);
		}
	}

	free(device);

	return header_printed ? 0 : COMMAND_ERROR;
}

BAREBOX_CMD_HELP_START(findmnt)
BAREBOX_CMD_HELP_TEXT("findmnt will list all mounted filesystems or search")
BAREBOX_CMD_HELP_TEXT("for a filesystem when given the mountpoint or the")
BAREBOX_CMD_HELP_TEXT("source device as an argument")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-T",  "mount target file path")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(findmnt)
	.cmd		= do_findmnt,
	BAREBOX_CMD_DESC("find a file system")
	BAREBOX_CMD_OPTS("[ DEVICE | -T FILE ]")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_findmnt_help)
BAREBOX_CMD_END

/*
 * erase, protect, unprotect - FLASH support
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file
 * @brief Flash memory support: erase, protect, unprotect
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <getopt.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>

static int do_flerase(int argc, char *argv[])
{
	int fd;
	char *filename = NULL;
	struct stat s;
	loff_t start = 0, size = ~0;
	int ret = 0;

	if (argc == 1)
		return COMMAND_ERROR_USAGE;

	filename = argv[1];

	if (stat(filename, &s)) {
		printf("stat %s: %s\n", filename, errno_str());
		return 1;
	}

	size = s.st_size;

	fd = open(filename, O_WRONLY);
	if (fd < 0) {
		printf("open %s: %s\n", filename, errno_str());
		return 1;
	}

	if (argc == 3 && parse_area_spec(argv[2], &start, &size)) {
		printf("could not parse: %s\n", argv[optind]);
		ret = 1;
		goto out;
	}

	if (erase(fd, size, start)) {
		perror("erase");
		ret = 1;
	}
out:
	close(fd);

	return ret;
}

BAREBOX_CMD_HELP_START(erase)
BAREBOX_CMD_HELP_TEXT("Erase the flash memory handled by DEVICE. Which AREA will be erased")
BAREBOX_CMD_HELP_TEXT("depends on the device: If the device represents the whole flash")
BAREBOX_CMD_HELP_TEXT("memory, the whole memory will be erased. If the device represents a")
BAREBOX_CMD_HELP_TEXT("partition on a main flash memory, only this partition part will be")
BAREBOX_CMD_HELP_TEXT("erased.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Use 'addpart' and 'delpart' to manage partitions.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(erase)
	.cmd		= do_flerase,
	BAREBOX_CMD_DESC("erase flash memory")
	BAREBOX_CMD_OPTS("DEVICE [AREA]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_erase_help)
BAREBOX_CMD_END

/**
 * @page erase_command

<p> Erase the flash memory handled by this device. Which area will be
erased depends on the device: If the device represents the whole flash
memory, the whole memory will be erased. If the device represents a
partition on a main flash memory, only this partition part will be
erased. </p>

Refer to \ref addpart_command, \ref delpart_command and \ref
devinfo_command for partition handling.

 */

static int do_protect(int argc, char *argv[])
{
	int fd;
	char *filename = NULL;
	struct stat s;
	int prot = 1;
	loff_t start = 0, size = ~0;
	int ret = 0, err;

	if (argc == 1)
		return COMMAND_ERROR_USAGE;

	filename = argv[1];

	if (*argv[0] == 'u')
		prot = 0;

	if (stat(filename, &s)) {
		printf("stat %s: %s\n", filename, errno_str());
		return 1;
	}

	size = s.st_size;

	fd = open(filename, O_WRONLY);
	if (fd < 0) {
		printf("open %s: %s\n", filename, errno_str());
		return 1;
	}

	if (argc == 3)
		if (parse_area_spec(argv[2], &start, &size)) {
			printf("could not parse: %s\n", argv[optind]);
			ret = 1;
			goto out;
		}

	err = protect(fd, size, start, prot);
        if (err && err != -ENOSYS) {
		perror("protect");
		ret = 1;
		goto out;
	}
out:
	close(fd);

	return ret;
}

BAREBOX_CMD_HELP_START(protect)
BAREBOX_CMD_HELP_TEXT("Protect the flash memory behind the device. It depends on the device")
BAREBOX_CMD_HELP_TEXT("given, what area will be protected. If the device represents the whole")
BAREBOX_CMD_HELP_TEXT("flash memory, the whole memory will be protected. If the device")
BAREBOX_CMD_HELP_TEXT("represents a partition on a main flash memory, only this partition part")
BAREBOX_CMD_HELP_TEXT("will be protected.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Use 'addpart' and 'delpart' to manage partitions.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(protect)
	.cmd		= do_protect,
	BAREBOX_CMD_DESC("enable flash write protection")
	BAREBOX_CMD_OPTS("DEVICE [AREA]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_protect_help)
BAREBOX_CMD_END

/**
 * @page protect_command


If the device represents the whole
flash memory the whole memory will be protected. If the device
represents a partition on a main flash memory, only this partition part
will be protected.

Refer addpart_command, delpart_command and devinfo_command for partition
handling.

\todo Rework this documentation, what is an 'area'? Explain more about
flashes here.

 */

BAREBOX_CMD_HELP_START(unprotect)
BAREBOX_CMD_HELP_TEXT("Unprotect the flash memory behind the device. It depends on the device")
BAREBOX_CMD_HELP_TEXT("given, what area will be unprotected. If the device represents the whole")
BAREBOX_CMD_HELP_TEXT("flash memory, the whole memory will be unprotected. If the device")
BAREBOX_CMD_HELP_TEXT("represents a partition on a main flash memory, only this partition part")
BAREBOX_CMD_HELP_TEXT("will be unprotected.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(unprotect)
	.cmd		= do_protect,
	BAREBOX_CMD_DESC("disable flash write protection")
	BAREBOX_CMD_OPTS("DEVICE [AREA]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_unprotect_help)
BAREBOX_CMD_END

/**
 * @page unprotect_command

It depends on the device given,
what area will be unprotected. If the device represents the whole flash memory
the whole memory will be unprotected. If the device represents a partition
on a main flash memory, only this partition part will be unprotected.

Refer addpart_command, delpart_command and devinfo_command for partition
handling.

\todo Rework this documentation, what does it mean?

 */


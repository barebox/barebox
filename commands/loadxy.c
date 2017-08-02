/**
 * @file
 * @brief loady and loadx  support.
 *
 * Provides loadx (over X-Modem) and loady(over Y-Modem) support to download
 * images.
 *
 * FileName: commands/loadxy.c
 */
/*
 * (C) Copyright 2012 Robert Jarzmik <robert.jarzmik@free.fr>
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

/*
 * Serial up- and download support
 */
#include <common.h>
#include <command.h>
#include <console.h>
#include <xymodem.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <fs.h>
#include <malloc.h>

#define DEF_FILE	"image.bin"

/**
 * @brief provide the loady(Y-Modem or Y-Modem/G) support
 *
 * @param argc number of arguments
 * @param argv arguments of loady command
 *
 * @return success or failure
 */
static int do_loady(int argc, char *argv[])
{
	int is_ymodemg = 0, rc = 0, opt, rcode = 0;
	int load_baudrate = 0, current_baudrate;
	char *cname = NULL;
	struct console_device *cdev = NULL;

	while ((opt = getopt(argc, argv, "b:t:g")) > 0) {
		switch (opt) {
		case 'b':
			load_baudrate = (int)simple_strtoul(optarg, NULL, 10);
			break;
		case 'g':
			is_ymodemg = 1;
			break;
		case 't':
			cname = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (cname)
		cdev = console_get_by_name(cname);
	else
		cdev = console_get_first_active();
	if (!cdev) {
		printf("%s:No console device %s with STDIN and STDOUT\n",
		       argv[0], cname ? cname : "default");
		return -ENODEV;
	}

	current_baudrate = console_get_baudrate(cdev);

	if (!load_baudrate)
		load_baudrate = current_baudrate;

	rc = console_set_baudrate(cdev, load_baudrate);
	if (rc)
		return rc;

	printf("## Ready for binary (ymodem) download at %d bps...\n",
	       load_baudrate ? load_baudrate : current_baudrate);

	if (is_ymodemg)
		rc = do_load_serial_ymodemg(cdev);
	else
		rc = do_load_serial_ymodem(cdev);

	if (rc < 0) {
		printf("## Binary (ymodem) download aborted (%d)\n", rc);
		rcode = 1;
	}

	rc = console_set_baudrate(cdev, current_baudrate);
	if (rc)
		return rc;

	return rcode;
}

BAREBOX_CMD_HELP_START(loady)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-g", "use Y-Modem/G (use on lossless tty such as USB)")
BAREBOX_CMD_HELP_OPT("-b BAUD", "baudrate for download (default: console baudrate)")
BAREBOX_CMD_HELP_OPT("-t NAME", "console name to use (default: current)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(loady)
	.cmd = do_loady,
	BAREBOX_CMD_DESC("load binary file over serial line (Y-Modem)")
	BAREBOX_CMD_OPTS("[-gtb]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_loady_help)
BAREBOX_CMD_END


/**
 * @brief provide the loadx(X-Modem) support
 *
 * @param argc number of arguments
 * @param argv arguments of loadx command
 *
 * @return success or failure
 */
static int do_loadx(int argc, char *argv[])
{
	ulong offset = 0;
	int load_baudrate = 0, current_baudrate, rc, ofd, opt, rcode = 0;
	char *output_file = NULL, *cname = NULL;
	struct console_device *cdev = NULL;

	while ((opt = getopt(argc, argv, "f:b:t:o:c")) > 0) {
		switch (opt) {
		case 'f':
			output_file = optarg;
			break;
		case 'b':
			load_baudrate = (int)simple_strtoul(optarg, NULL, 10);
			break;
		case 'o':
			offset = (int)simple_strtoul(optarg, NULL, 10);
			break;
		case 't':
			cname = optarg;
			break;
		default:
			perror(argv[0]);
			return 1;
		}
	}

	if (cname)
		cdev = console_get_by_name(cname);
	else
		cdev = console_get_first_active();
	if (!cdev) {
		printf("%s:No console device %s with STDIN and STDOUT",
		       argv[0], cname ? cname : "default");
		return -ENODEV;
	}

	/* Load Defaults */
	if (!output_file)
		output_file = DEF_FILE;

	/* File should exist */
	ofd = open(output_file, O_WRONLY | O_CREAT);
	if (ofd < 0) {
		perror(argv[0]);
		return 3;
	}
	/* Seek to the right offset */
	if (offset) {
		int seek = lseek(ofd, offset, SEEK_SET);
		if (seek != offset) {
			close(ofd);
			ofd = 0;
			perror(argv[0]);
			return 4;
		}
	}

	current_baudrate = console_get_baudrate(cdev);

	if (!load_baudrate)
		load_baudrate = current_baudrate;

	rc = console_set_baudrate(cdev, load_baudrate);
	if (rc)
		return rc;

	printf("## Ready for binary (xmodem) download "
	       "to 0x%08lX offset on %s device at %d bps...\n", offset,
	       output_file, load_baudrate ? load_baudrate : current_baudrate);
	rcode = do_load_serial_xmodem(cdev, ofd);
	if (rcode < 0) {
		printf("## Binary (xmodem) download aborted (%d)\n", rcode);
		rcode = 1;
	}

	rc = console_set_baudrate(cdev, current_baudrate);
	if (rc)
		return rc;

	return rcode;
}

BAREBOX_CMD_HELP_START(loadx)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f FILE", "download to FILE (default image.bin")
BAREBOX_CMD_HELP_OPT("-o OFFS", "destination file OFFSet (default 0)")
BAREBOX_CMD_HELP_OPT("-b BAUD", "baudrate for download (default: console baudrate)")
BAREBOX_CMD_HELP_OPT("-t NAME", "console name to use (default: current)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(loadx)
	.cmd = do_loadx,
	BAREBOX_CMD_DESC("load binary file over serial line (X-Modem)")
	BAREBOX_CMD_OPTS("[-fptbc]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_loadx_help)
BAREBOX_CMD_END

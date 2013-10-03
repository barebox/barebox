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

static int console_change_speed(struct console_device *cdev, int baudrate)
{
	int current_baudrate;

	current_baudrate =
		(int)simple_strtoul(dev_get_param(&cdev->class_dev,
						  "baudrate"), NULL, 10);
	if (baudrate && baudrate != current_baudrate) {
		printf("## Switch baudrate from %d to %d bps and press ENTER ...\n",
		       current_baudrate, baudrate);
		mdelay(50);
		cdev->setbrg(cdev, baudrate);
		mdelay(50);
	}
	return current_baudrate;
}

static struct console_device *get_named_console(const char *cname)
{
	struct console_device *cdev;
	const char *target;

	/*
	 * Assumption to have BOTH CONSOLE_STDIN AND STDOUT in the
	 * same output console
	 */
	for_each_console(cdev) {
		target = dev_id(&cdev->class_dev);
		if (strlen(target) != strlen(cname))
			continue;
		printf("RJK: looking for %s in console name %s\n",
		       cname, target);
		if ((cdev->f_active & (CONSOLE_STDIN | CONSOLE_STDOUT))
		    && !strcmp(cname, target))
			return cdev;
	}
	return NULL;
}

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
			perror(argv[0]);
			return 1;
		}
	}

	if (cname)
		cdev = get_named_console(cname);
	else
		cdev = console_get_first_active();
	if (!cdev) {
		printf("%s:No console device %s with STDIN and STDOUT\n",
		       argv[0], cname ? cname : "default");
		return -ENODEV;
	}

	current_baudrate = console_change_speed(cdev, load_baudrate);
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

	console_change_speed(cdev, current_baudrate);

	return rcode;
}

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
	int load_baudrate = 0, current_baudrate, ofd, opt, rcode = 0;
	int open_mode = O_WRONLY;
	char *output_file = NULL, *cname = NULL;
	struct console_device *cdev = NULL;

	while ((opt = getopt(argc, argv, "f:b:o:c")) > 0) {
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
		case 'c':
			open_mode |= O_CREAT;
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
		cdev = get_named_console(cname);
	else
		cdev = console_get_first_active();
	if (!cdev) {
		printf("%s:No console device %s with STDIN and STDOUT\n",
		       argv[0], cname ? cname : "default");
		return -ENODEV;
	}

	/* Load Defaults */
	if (!output_file)
		output_file = DEF_FILE;

	/* File should exist */
	ofd = open(output_file, open_mode);
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

	current_baudrate = console_change_speed(cdev, load_baudrate);
	printf("## Ready for binary (X-Modem) download "
	       "to 0x%08lX offset on %s device at %d bps...\n", offset,
	       output_file, load_baudrate);
	rcode = do_load_serial_ymodem(cdev);
	if (rcode < 0) {
		printf("## Binary (kermit) download aborted (%d)\n", rcode);
		rcode = 1;
	}
	console_change_speed(cdev, current_baudrate);

	return rcode;
}

static const __maybe_unused char cmd_loadx_help[] =
	"[OPTIONS]\n"
	"  -f file   - where to download to - defaults to " DEF_FILE "\n"
	"  -o offset - what offset to download - defaults to 0\n"
	"  -t name   - console device name to use - defaults to current console\n"
	"  -b baud   - baudrate at which to download - defaults to console baudrate\n"
	"  -c        - Create file if it is not present - default disabled";

#ifdef CONFIG_CMD_LOADY
BAREBOX_CMD_START(loadx)
	.cmd = do_loadx,
	.usage = "Load binary file over serial line (X-Modem)",
BAREBOX_CMD_HELP(cmd_loadx_help)
BAREBOX_CMD_END

static const __maybe_unused char cmd_loady_help[] =
	"[OPTIONS]\n"
	"  -y        - use Y-Modem/G (only for lossless tty as USB)\n"
	"  -t name   - console device name to use - defaults to current console\n"
	"  -b baud   - baudrate at which to download - defaults to console baudrate\n";

BAREBOX_CMD_START(loady)
	.cmd = do_loady,
	.usage = "Load binary file over serial line (Y-Modem or Y-Modem/G)",
BAREBOX_CMD_HELP(cmd_loady_help)
BAREBOX_CMD_END
#endif

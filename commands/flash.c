/*
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * FLASH support
 */
#include <common.h>
#include <command.h>
#include <cfi_flash.h>
#include <errno.h>
#include <getopt.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>

int do_flerase (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int fd;
	char *filename = NULL;
	struct stat s;
	unsigned long start = 0, size = ~0;

	if (argc == 1) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	filename = argv[1];

	if (stat(filename, &s)) {
		printf("stat %s: %s\n", filename, errno_str());
		return 1;
	}

	size = s.st_size;

	if (!filename) {
		printf("missing filename\n");
		return 1;
	}

	fd = open(filename, O_WRONLY);
	if (fd < 0) {
		printf("open %s:", filename, errno_str());
		return 1;
	}

	if (argc == 2)
		parse_area_spec(argv[optind], &start, &size);

	if(erase(fd, size, start)) {
		perror("erase");
		return 1;
	}

	close(fd);

	return 0;
}

static __maybe_unused char cmd_erase_help[] =
"Usage: Erase <device> [area]\n"
"Erase a flash device or parts of a device if an area specification\n"
"is given\n";

U_BOOT_CMD_START(erase)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_flerase,
	.usage		= "erase FLASH memory",
	U_BOOT_CMD_HELP(cmd_erase_help)
U_BOOT_CMD_END

int do_protect (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int fd;
	char *filename = NULL;
	struct stat s;
	int prot = 1;
	unsigned long start = 0, size = ~0;

	if (argc == 1) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	filename = argv[1];

	if (*argv[0] == 'u')
		prot = 0;

	if (stat(filename, &s)) {
		printf("stat %s: %s\n", filename, errno_str());
		return 1;
	}

	size = s.st_size;

	if (!filename) {
		printf("missing filename\n");
		return 1;
	}

	fd = open(filename, O_WRONLY);
	if (fd < 0) {
		printf("open %s:", filename, errno_str());
		return 1;
	}

	if (argc == 3)
		parse_area_spec(argv[optind], &start, &size);

        if(protect(fd, size, start, prot)) {
		perror("protect");
		return 1;
	}

	close(fd);

	return 0;
}

static __maybe_unused char cmd_protect_help[] =
"Usage: (un)protect <device> [area]\n"
"(un)protect a flash device or parts of a device if an area specification\n"
"is given\n";

U_BOOT_CMD_START(protect)
	.maxargs	= 4,
	.cmd		= do_protect,
	.usage		= "enable FLASH write protection",
	U_BOOT_CMD_HELP(cmd_protect_help)
U_BOOT_CMD_END

U_BOOT_CMD_START(unprotect)
	.maxargs	= 4,
	.cmd		= do_protect,
	.usage		= "disable FLASH write protection",
	U_BOOT_CMD_HELP(cmd_protect_help)
U_BOOT_CMD_END


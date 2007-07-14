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
	int opt, fd;
	char *filename = NULL;
	struct stat s;
	unsigned long start = 0, size = ~0;

	getopt_reset();

	while((opt = getopt(argc, argv, "f:")) > 0) {
		switch(opt) {
		case 'f':
			filename = optarg;
			break;
		}
	}

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

	if (optind  < argc)
		parse_area_spec(argv[optind], &start, &size);

        if(erase(fd, size, start)) {
		perror("erase");
		return 1;
	}

	close(fd);

	return 0;
}

U_BOOT_CMD_START(erase)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_flerase,
	.usage		= "erase FLASH memory",
	U_BOOT_CMD_HELP("write me\n")
U_BOOT_CMD_END

int do_protect (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int p;
	int rcode = 0;
	struct memarea_info mem_info;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp(argv[1], "off") == 0) {
		p = 0;
	} else if (strcmp(argv[1], "on") == 0) {
		p = 1;
	} else {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (spec_str_to_info(argv[2], &mem_info)) {
		printf ("-EPARSE\n");
		return 1;
	}

//	rcode = flash_sect_protect (p, addr_first, addr_last);
	return rcode;
}

U_BOOT_CMD_START(protect)
	.maxargs	= 4,
	.cmd		= do_protect,
	.usage		= "enable or disable FLASH write protection",
	U_BOOT_CMD_HELP("write me\n")
U_BOOT_CMD_END

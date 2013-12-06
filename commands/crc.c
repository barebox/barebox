/*
 * crc.c - Calculate a crc32 checksum of a memory area
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
#include <getopt.h>
#include <malloc.h>
#include <environment.h>

static int crc_from_file(const char* file, ulong *crc)
{
	char * buf;

	buf= read_file(file, NULL);

	if (!buf)
		return -ENOMEM;

	*crc = simple_strtoul(buf, NULL, 16);
	return 0;
}

static int do_crc(int argc, char *argv[])
{
	loff_t start = 0, size = ~0;
	ulong crc = 0, vcrc = 0, total = 0;
	char *filename = "/dev/mem";
#ifdef CONFIG_CMD_CRC_CMP
	char *vfilename = NULL;
#endif
	int opt, err = 0, filegiven = 0, verify = 0;

	while((opt = getopt(argc, argv, "f:F:v:V:")) > 0) {
		switch(opt) {
		case 'f':
			filename = optarg;
			filegiven = 1;
			break;
#ifdef CONFIG_CMD_CRC_CMP
		case 'F':
			verify = 1;
			vfilename = optarg;
			break;
#endif
		case 'v':
			verify = 1;
			vcrc = simple_strtoul(optarg, NULL, 0);
			break;
		case 'V':
			if (!crc_from_file(optarg, &vcrc))
				verify = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!filegiven && optind == argc)
		return COMMAND_ERROR_USAGE;

	if (optind < argc) {
		if (parse_area_spec(argv[optind], &start, &size)) {
			printf("could not parse area description: %s\n", argv[optind]);
			return 1;
		}
	}

	if (file_crc(filename, start, size, &crc, &total) < 0)
		return 1;

	printf("CRC32 for %s 0x%08lx ... 0x%08lx ==> 0x%08lx",
			filename, (ulong)start, (ulong)start + total - 1, crc);

#ifdef CONFIG_CMD_CRC_CMP
	if (vfilename) {
		size = total;
		puts("\n");
		if (file_crc(vfilename, start, size, &vcrc, &total) < 0)
			return 1;
	}
#endif

	if (verify && crc != vcrc) {
		printf(" != 0x%08lx ** ERROR **", vcrc);
		err = 1;
	}

	printf("\n");

	return err;
}

BAREBOX_CMD_HELP_START(crc)
BAREBOX_CMD_HELP_USAGE("crc32 [OPTION] [AREA]\n")
BAREBOX_CMD_HELP_SHORT("Calculate a crc32 checksum of a memory area.\n")
BAREBOX_CMD_HELP_OPT  ("-f <file>", "Use file instead of memory.\n")
#ifdef CONFIG_CMD_CRC_CMP
BAREBOX_CMD_HELP_OPT  ("-F <file>", "Use file to compare.\n")
#endif
BAREBOX_CMD_HELP_OPT  ("-v <crc>",  "Verify\n")
BAREBOX_CMD_HELP_OPT  ("-V <file>", "Verify with crc read from <file>\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(crc32)
	.cmd		= do_crc,
	.usage		= "crc32 checksum calculation",
	BAREBOX_CMD_HELP(cmd_crc_help)
BAREBOX_CMD_END

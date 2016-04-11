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
#include <crc.h>
#include <getopt.h>
#include <malloc.h>
#include <libfile.h>
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
	char *crcvarname = NULL, *sizevarname = NULL;
	int opt, err = 0, filegiven = 0, verify = 0;

	while((opt = getopt(argc, argv, "f:F:v:V:r:s:")) > 0) {
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
		case 'r':
			crcvarname = optarg;
			break;
		case 's':
			sizevarname = optarg;
			break;
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

	if (crcvarname) {
		char *crcstr = basprintf("0x%lx", crc);
		setenv(crcvarname, crcstr);
		kfree(crcstr);
	}

	if (sizevarname) {
		char *sizestr = basprintf("0x%lx", total);
		setenv(sizevarname, sizestr);
		kfree(sizestr);
	}

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

BAREBOX_CMD_HELP_START(crc32)
BAREBOX_CMD_HELP_TEXT("Calculate a CRC32 checksum of a memory area.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-f FILE", "Use file instead of memory.")
#ifdef CONFIG_CMD_CRC_CMP
BAREBOX_CMD_HELP_OPT ("-F FILE", "Use file to compare.")
#endif
BAREBOX_CMD_HELP_OPT ("-v CRC",  "Verify")
BAREBOX_CMD_HELP_OPT ("-V FILE", "Verify with CRC read from FILE")
BAREBOX_CMD_HELP_OPT  ("-r <var>",  "Set <var> to the checksum result\n")
BAREBOX_CMD_HELP_OPT  ("-s <var>",  "Set <var> to the data size\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(crc32)
	.cmd		= do_crc,
	BAREBOX_CMD_DESC("CRC32 checksum calculation")
	BAREBOX_CMD_OPTS("[-f"
#ifdef CONFIG_CMD_CRC_CMP
					  "F"
#endif
					  "vV] AREA")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_crc32_help)
BAREBOX_CMD_END

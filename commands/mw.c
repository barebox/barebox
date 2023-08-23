// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <libfile.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

static int do_mem_mw(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	char *filename = "/dev/mem";
	int mode = O_RWSIZE_4;
	loff_t adr;
	int swab = 0;

	if (mem_parse_options(argc, argv, "bwlqd:x", &mode, NULL, &filename,
			&swab) < 0)
		return 1;

	if (optind + 1 >= argc)
		return COMMAND_ERROR_USAGE;

	adr = strtoull_suffix(argv[optind++], NULL, 0);

	fd = open_and_lseek(filename, mode | O_WRONLY | O_CREAT, adr);
	if (fd < 0) {
		printf("Could not open \"%s\": %m\n", filename);
		return 1;
	}

	while (optind < argc) {
		u8 val8;
		u16 val16;
		u32 val32;
		u64 val64;
		switch (mode) {
		case O_RWSIZE_1:
			ret = kstrtou8(argv[optind], 0, &val8);
			if (ret)
				goto illegal_number;
			ret = write(fd, &val8, 1);
			break;
		case O_RWSIZE_2:
			ret = kstrtou16(argv[optind], 0, &val16);
			if (ret)
				goto illegal_number;
			if (swab)
				val16 = __swab16(val16);
			ret = write(fd, &val16, 2);
			break;
		case O_RWSIZE_4:
			ret = kstrtou32(argv[optind], 0, &val32);
			if (ret)
				goto illegal_number;
			if (swab)
				val32 = __swab32(val32);
			ret = write(fd, &val32, 4);
			break;
		case O_RWSIZE_8:
			ret = kstrtou64(argv[optind], 0, &val64);
			if (ret)
				goto illegal_number;
			if (swab)
				val64 = __swab64(val64);
			ret = write(fd, &val64, 8);
			break;
		}
		if (ret < 0) {
			perror("write");
			break;
		}
		ret = 0;
		optind++;
	}

	close(fd);

	return ret ? 1 : 0;

illegal_number:
	printf("\"%s\" is not a valid %d bit number\n", argv[optind],
	       (mode >> O_RWSIZE_SHIFT) * 8);

	close(fd);

	return 1;
}

BAREBOX_CMD_HELP_START(mw)
BAREBOX_CMD_HELP_TEXT("Write DATA value(s) to the specified REGION.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b",  "byte access")
BAREBOX_CMD_HELP_OPT ("-w",  "word access (16 bit)")
BAREBOX_CMD_HELP_OPT ("-l",  "long access (32 bit)")
BAREBOX_CMD_HELP_OPT ("-q",  "quad access (64 bit)")
BAREBOX_CMD_HELP_OPT ("-d FILE",  "write file (default /dev/mem)")
BAREBOX_CMD_HELP_OPT ("-x",       "swap bytes")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Memory regions can be specified in two different forms: START+SIZE")
BAREBOX_CMD_HELP_TEXT("or START-END, If START is omitted it defaults to 0x100")
BAREBOX_CMD_HELP_TEXT("Sizes can be specified as decimal, or if prefixed with 0x as hexadecimal.")
BAREBOX_CMD_HELP_TEXT("An optional suffix of k, M or G is for kbytes, Megabytes or Gigabytes.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mw)
	.cmd		= do_mem_mw,
	BAREBOX_CMD_DESC("memory write")
	BAREBOX_CMD_OPTS("[-bwlqx] [-d FILE] REGION DATA...")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_mw_help)
BAREBOX_CMD_END

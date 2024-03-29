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

static int do_mem_md(int argc, char *argv[])
{
	loff_t	start = 0, size = 0x100;
	int	r, now;
	int	ret = 0;
	int fd;
	char *filename = "/dev/mem";
	int mode = O_RWSIZE_4;
	int swab = 0;
	void *map;
	void *buf = NULL;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	if (mem_parse_options(argc, argv, "bwlqs:x", &mode, &filename, NULL,
			&swab) < 0)
		return 1;

	if (optind < argc) {
		if (parse_area_spec(argv[optind], &start, &size)) {
			printf("could not parse: %s\n", argv[optind]);
			return 1;
		}
		if (size == ~0)
			size = 0x100;
	}

	fd = open_and_lseek(filename, mode | O_RDONLY, start);
	if (fd < 0) {
		printf("Could not open \"%s\": %m\n", filename);
		return 1;
	}

	map = memmap(fd, PROT_READ);
	if (map != MAP_FAILED) {
		ret = memory_display(map + start, start, size,
				mode >> O_RWSIZE_SHIFT, swab);
		goto out;
	}

	buf = xzalloc(RW_BUF_SIZE + 7);

	do {
		now = min(size, (loff_t)RW_BUF_SIZE);
		r = read(fd, buf, now);
		if (r < 0) {
			perror("read");
			goto out;
		}
		if (!r)
			goto out;

		if ((ret = memory_display(buf, start, r,
					  mode >> O_RWSIZE_SHIFT, swab)))
			goto out;

		start += r;
		size  -= r;
	} while (size);

out:
	free(buf);
	close(fd);

	return ret ? 1 : 0;
}


BAREBOX_CMD_HELP_START(md)
BAREBOX_CMD_HELP_TEXT("Display (hex dump) a memory REGION.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b",  "byte access")
BAREBOX_CMD_HELP_OPT ("-w",  "word access (16 bit)")
BAREBOX_CMD_HELP_OPT ("-l",  "long access (32 bit)")
BAREBOX_CMD_HELP_OPT ("-q",  "quad access (64 bit)")
BAREBOX_CMD_HELP_OPT ("-s FILE",  "display file (default /dev/mem)")
BAREBOX_CMD_HELP_OPT ("-x",       "swap bytes at output")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Memory regions can be specified in two different forms: START+SIZE")
BAREBOX_CMD_HELP_TEXT("or START-END, If START is omitted it defaults to 0x100")
BAREBOX_CMD_HELP_TEXT("Sizes can be specified as decimal, or if prefixed with 0x as hexadecimal.")
BAREBOX_CMD_HELP_TEXT("An optional suffix of k, M or G is for kbytes, Megabytes or Gigabytes.")
BAREBOX_CMD_HELP_END


BAREBOX_CMD_START(md)
	.cmd		= do_mem_md,
	BAREBOX_CMD_DESC("memory display")
	BAREBOX_CMD_OPTS("[-bwlqx] [-s FILE] REGION")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_md_help)
BAREBOX_CMD_END

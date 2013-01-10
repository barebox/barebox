/*
 * oftree.c - device tree command support
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on U-Boot code by:
 *
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 * Pantelis Antoniou <pantelis.antoniou@gmail.com> and
 * Matthew McClintock <msm@freescale.com>
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
#include <environment.h>
#include <fdt.h>
#include <of.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <libfdt.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <init.h>

static int do_oftree(int argc, char *argv[])
{
	struct fdt_header *fdt;
	int size;
	int opt;
	char *file = NULL;
	const char *node = "/";
	int dump = 0;
	int probe = 0;
	int ret;

	while ((opt = getopt(argc, argv, "dpfn:")) > 0) {
		switch (opt) {
		case 'd':
			dump = 1;
			break;
		case 'p':
			if (IS_ENABLED(CONFIG_CMD_OFTREE_PROBE)) {
				probe = 1;
			} else {
				printf("oftree device probe support disabled\n");
				return COMMAND_ERROR_USAGE;
			}
			break;
		case 'f':
			return 0;
		case 'n':
			node = optarg;
			break;
		}
	}

	if (optind < argc)
		file = argv[optind];

	if (!dump && !probe)
		return COMMAND_ERROR_USAGE;

	if (dump) {
		if (file) {
			fdt = read_file(file, &size);
			if (!fdt) {
				printf("unable to read %s\n", file);
				return 1;
			}

			fdt_print(fdt, node);
			free(fdt);
		} else {
			return 1;
		}
		return 0;
	}

	if (probe) {
		if (!file)
			return COMMAND_ERROR_USAGE;

		fdt = read_file(file, &size);
		if (!fdt) {
			perror("open");
			return 1;
		}

		ret = of_unflatten_dtb(fdt);
		if (ret) {
			printf("parse oftree: %s\n", strerror(-ret));
			return 1;
		}

		of_probe();
	}

	return 0;
}

BAREBOX_CMD_HELP_START(oftree)
BAREBOX_CMD_HELP_USAGE("oftree [OPTIONS]\n")
BAREBOX_CMD_HELP_OPT  ("-p <FILE>",  "probe devices in oftree from <file>\n")
BAREBOX_CMD_HELP_OPT  ("-d [FILE]",  "dump oftree from [FILE] or the parsed tree if no file is given\n")
BAREBOX_CMD_HELP_OPT  ("-f",  "free stored oftree\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(oftree)
	.cmd		= do_oftree,
	.usage		= "handle oftrees",
	BAREBOX_CMD_HELP(cmd_oftree_help)
BAREBOX_CMD_END

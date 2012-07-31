/*
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

/**
 * @file
 * @brief Concatenate files to stdout command
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/ctype.h>
#include <errno.h>
#include <xfuncs.h>
#include <malloc.h>

#define BUFSIZE 1024

/**
 * @param[in] argc Argument count from command line
 * @param[in] argv List of input arguments
 */
static int do_cat(int argc, char *argv[])
{
	int ret;
	int fd, i;
	char *buf;
	int err = 0;
	int args = 1;

	if (argc < 2) {
		perror("cat");
		return 1;
	}

	buf = xmalloc(BUFSIZE);

	while (args < argc) {
		fd = open(argv[args], O_RDONLY);
		if (fd < 0) {
			err = 1;
			printf("could not open %s: %s\n", argv[args], errno_str());
			goto out;
		}

		while((ret = read(fd, buf, BUFSIZE)) > 0) {
			for(i = 0; i < ret; i++) {
				if (isprint(buf[i]) || buf[i] == '\n' || buf[i] == '\t')
					putchar(buf[i]);
				else
					putchar('.');
			}
			if(ctrlc()) {
				err = 1;
				close(fd);
				goto out;
			}
		}
		close(fd);
		args++;
	}

out:
	free(buf);

	return err;
}

BAREBOX_CMD_HELP_START(cat)
BAREBOX_CMD_HELP_USAGE("cat [FILES]\n")
BAREBOX_CMD_HELP_SHORT("Concatenate files on stdout.\n")
BAREBOX_CMD_HELP_TEXT ("Currently only printable characters and \\ n and \\ t are printed,\n")
BAREBOX_CMD_HELP_TEXT ("but this should be optional.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(cat)
	.cmd		= do_cat,
	.usage		= "concatenate file(s)",
	BAREBOX_CMD_HELP(cmd_cat_help)
BAREBOX_CMD_END

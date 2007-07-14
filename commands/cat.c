/*
 * cat.c - conacatenate files
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/ctype.h>
#include <errno.h>
#include <xfuncs.h>
#include <malloc.h>

static int do_cat(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

	buf = xmalloc(1024);

	while (args < argc) {
		fd = open(argv[args], O_RDONLY);
		if (fd < 0) {
			printf("could not open %s: %s\n", argv[args], errno_str());
			goto out;
		}

		while((ret = read(fd, buf, 1024)) > 0) {
			for(i = 0; i < ret; i++) {
				if (isprint(buf[i]) || buf[i] == '\n' || buf[i] == '\t')
					putc(buf[i]);
				else
					putc('.');
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

static __maybe_unused char cmd_cat_help[] =
"Usage: cat [FILES]\n"
"Concatenate files on stdout. Currently only printable characters\n"
"and \\n and \\t are printed, but this should be optional\n";

U_BOOT_CMD_START(cat)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_cat,
	.usage		= "concatenate file(s)",
	U_BOOT_CMD_HELP(cmd_cat_help)
U_BOOT_CMD_END

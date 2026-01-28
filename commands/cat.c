// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

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
#include <getopt.h>
#include <libfile.h>

#define BUFSIZE 1024

/**
 * @param[in] argc Argument count from command line
 * @param[in] argv List of input arguments
 */
static int do_cat(int argc, char *argv[])
{
	const char *outfile = NULL;
	int ret;
	int fd, outfd = -1, i;
	char *buf;
	int err = 0;
	int oflags = O_WRONLY | O_CREAT;
	int opt;

	while ((opt = getopt(argc, argv, "a:o:")) > 0) {
		switch(opt) {
		case 'a':
			oflags |= O_APPEND;
			outfile = optarg;
			break;
		case 'o':
			oflags |= O_TRUNC;
			outfile = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc == 0)
		return COMMAND_ERROR_USAGE;

	if (outfile) {
		outfd = open(outfile, oflags);
		if (outfd < 0) {
			perror("open");
			return 1;
		}
	}

	buf = xmalloc(BUFSIZE);

	for (int args = 0; args < argc; args++) {
		fd = open(argv[args], O_RDONLY);
		if (fd < 0) {
			err = 1;
			printf("could not open %s: %m\n", argv[args]);
			goto out;
		}


		if (outfd >= 0) {
			ret = copy_fd(fd, outfd, 0);
			if (ret < 0) {
				perror("copy_fd");
				err = 1;
				close(fd);
				goto out;
			}
		} else {
			while((ret = read(fd, buf, BUFSIZE)) > 0) {
				for (i = 0; i < ret; i++) {
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
		}

		close(fd);
	}

out:
	close(outfd);
	free(buf);

	return err;
}

BAREBOX_CMD_HELP_START(cat)
BAREBOX_CMD_HELP_TEXT("Currently only printable characters and NL, TAB are printed.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a FILE", "append to FILE instead of using stdout")
BAREBOX_CMD_HELP_OPT ("-o FILE", "overwrite FILE instead of using stdout")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(cat)
	.cmd		= do_cat,
	BAREBOX_CMD_DESC("concatenate file(s) to stdout")
	BAREBOX_CMD_OPTS("[-ao] FILE...")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_cat_help)
BAREBOX_CMD_END

// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2000-2003 Wolfgang Denk <wd@denx.de>, DENX Software Engineering

/* go - execute some code anywhere (misc boot support) */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/ctype.h>
#include <errno.h>

#define INT_ARGS_MAX	4

static int do_go(int argc, char *argv[])
{
	void	*addr;
	int     rcode = 1;
	int	fd = -1;
	void	(*func)(ulong r0, ulong r1, ulong r2, ulong r3);
	int	opt;
	ulong	arg[INT_ARGS_MAX] = {};
	bool	pass_argv = true;

	while ((opt = getopt(argc, argv, "+si")) > 0) {
		switch (opt) {
		case 's':
			pass_argv = true;
			break;
		case 'i':
			pass_argv = false;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	/* skip options */
	argv += optind;
	argc -= optind;

	if (argc == 0 || (!pass_argv && argc - 1 > INT_ARGS_MAX))
		return COMMAND_ERROR_USAGE;

	if (!isdigit(*argv[0])) {
		fd = open(argv[0], O_RDONLY);
		if (fd < 0) {
			perror("open");
			goto out;
		}

		addr = memmap(fd, PROT_READ);
		if (addr == MAP_FAILED) {
			perror("memmap");
			goto out;
		}
	} else
		addr = (void *)simple_strtoul(argv[0], NULL, 16);

	printf("## Starting application at 0x%p ...\n", addr);

	console_flush();

	func = addr;

	if (pass_argv) {
		arg[0] = argc;
		arg[1] = (ulong)argv;
	} else {
		int i;

		/* skip argv[0] */
		argc--;
		argv++;

		for (i = 0; i < argc; i++) {
			rcode = kstrtoul(argv[i], 0, &arg[i]);
			if (rcode) {
				printf("Can't parse \"%s\" as an integer value\n", argv[i]);
				goto out;
			}
		}
	}

	shutdown_barebox();

	func(arg[0], arg[1], arg[2], arg[3]);

	/*
	 * The application returned. Since we have shutdown barebox and
	 * we know nothing about the state of the cpu/memory we can't
	 * do anything here.
	 */
	while (1);
out:
	if (fd > 0)
		close(fd);

	return rcode;
}

BAREBOX_CMD_HELP_START(go)
BAREBOX_CMD_HELP_TEXT("Start application at ADDR passing ARG as arguments.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("If addr does not start with a digit it is interpreted as a filename")
BAREBOX_CMD_HELP_TEXT("in which case the file is memmapped and executed")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-s", "pass all arguments as strings referenced by argc, argv arguments (default)")
BAREBOX_CMD_HELP_OPT("-i", "pass up to four integer arguments using default calling convention")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(go)
	.cmd		= do_go,
	BAREBOX_CMD_DESC("start application at address or file")
	BAREBOX_CMD_OPTS("[-si] ADDR [ARG...]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_go_help)
	BAREBOX_CMD_COMPLETE(command_var_complete)
BAREBOX_CMD_END

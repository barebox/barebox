// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* echo.c - echo arguments to stdout or into a file */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <libbb.h>

static void echo_dputc(int fd, char c, bool wide)
{
	wchar_t wc;
	int n;

	if (!wide || fd == 1 || fd == 2) {
		dputc(fd, c);
		return;
	}

	n = mbtowc(&wc, &c, 1);
	if (n < 0)
		return;

	write(fd, &wc, sizeof(wchar_t));
}

static void echo_dputs(int fd, const char *s, bool wide)
{
	wchar_t *ws;

	if (!wide || fd == 1 || fd == 2) {
		dputs(fd, s);
		return;
	}

	ws = strdup_char_to_wchar(s);
	if (!ws)
		return;

	write(fd, ws, wcslen(ws) * sizeof(wchar_t));
	free(ws);
}

static int do_echo(int argc, char *argv[])
{
	int i, optind = 1;
	int fd = STDOUT_FILENO, opt, newline = 1;
	char *file = NULL;
	int oflags = O_WRONLY | O_CREAT;
	char str[CONFIG_CBSIZE];
	int process_escape = 0;
	bool wide = false;

	if (IS_ENABLED(CONFIG_PRINTF_WCHAR) && *argv[0] == 'w')
		wide = true;

	/* We can't use getopt() here because we want to
	 * echo all things we don't understand.
	 */
	while (optind < argc && *argv[optind] == '-') {
		if (!*(argv[optind] + 1) || *(argv[optind] + 2))
			break;

		opt = *(argv[optind] + 1);
		switch (opt) {
		case 'n':
			newline = 0;
			break;
		case 'a':
			oflags |= O_APPEND;
			if (optind + 1 < argc)
				file = argv[optind + 1];
			else
				goto no_optarg_out;
			optind++;
			break;
		case 'o':
			oflags |= O_TRUNC;
			if (optind + 1 < argc)
				file = argv[optind + 1];
			else
				goto no_optarg_out;
			optind++;
			break;
		case 'e':
			process_escape = IS_ENABLED(CONFIG_CMD_ECHO_E);
			break;
		default:
			goto exit_parse;
		}
		optind++;
	}

exit_parse:
	if (file) {
		fd = open(file, oflags);
		if (fd < 0) {
			perror("open");
			return 1;
		}
	}

	for (i = optind; i < argc; i++) {
		const char *out;

		if (i > optind)
			echo_dputc(fd, ' ', wide);
		if (process_escape) {
			process_escape_sequence(argv[i], str, CONFIG_CBSIZE);
			out = str;
		} else {
			out = argv[i];
		}

		echo_dputs(fd, out, wide);
	}

	if (newline)
		echo_dputc(fd, '\n', wide);

	if (file)
		close(fd);

	return 0;

no_optarg_out:
	printf("option requires an argument -- %c\n", opt);
	return 1;
}

BAREBOX_CMD_HELP_START(echo)
BAREBOX_CMD_HELP_TEXT("Display a line of TEXT on the console.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-n",     "do not output the trailing newline")
BAREBOX_CMD_HELP_OPT ("-e",     "recognize escape sequences")
BAREBOX_CMD_HELP_OPT ("-a FILE", "append to FILE instead of using stdout")
BAREBOX_CMD_HELP_OPT ("-o FILE", "overwrite FILE instead of using stdout")
BAREBOX_CMD_HELP_END

static __maybe_unused const char * const echo_aliases[] = { "wecho", NULL};

BAREBOX_CMD_START(echo)
	.cmd		= do_echo,
#ifdef CONFIG_PRINTF_WCHAR
	.aliases	= echo_aliases,
#endif
	BAREBOX_CMD_DESC("echo args to console")
	BAREBOX_CMD_OPTS("[-neao] STRING")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_echo_help)
BAREBOX_CMD_END

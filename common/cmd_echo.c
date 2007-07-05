#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>

static int do_echo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i, optind = 1;
        int fd = stdout, opt, newline = 1;
        char *file = NULL;
	int oflags = O_WRONLY | O_CREAT;

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
			file = argv[optind + 1];
			if (optind + 1 < argc)
				file = argv[optind + 1];
			else
				goto no_optarg_out;
			optind++;
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
		if (i > optind)
			fputc(fd, ' ');
		fputs(fd, argv[i]);
	}

	if (newline)
		fputc(fd, '\n');

	if (file)
		close(fd);

	return 0;

no_optarg_out:
	printf("option requires an argument -- %c\n", opt);
	return 1;
}

U_BOOT_CMD_START(echo)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_echo,
	.usage		= "echo    - echo args to console\n",
U_BOOT_CMD_END


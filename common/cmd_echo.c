#include <common.h>
#include <command.h>
#include <getopt.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>

static int do_echo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
        int fd = stdout, opt, newline = 1;
        char *file = NULL;
	int oflags = O_WRONLY | O_CREAT;
	getopt_reset();

	while((opt = getopt(argc, argv, "a:o:n")) > 0) {
		switch(opt) {
		case 'n':
			newline = 0;
			break;
		case 'a':
			oflags |= O_APPEND;
			file = optarg;
			break;
		case 'o':
			oflags |= O_TRUNC;
			file = optarg;
			break;
		}
	}

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
}

U_BOOT_CMD(
	echo,	CONFIG_MAXARGS,	0,	do_echo,
 	"echo    - echo args to console\n",
 	"[args..]\n"
	"    - echo args to console; \\c suppresses newline\n"
);


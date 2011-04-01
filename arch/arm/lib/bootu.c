#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <asm/armlinux.h>

static int do_bootu(struct command *cmdtp, int argc, char *argv[])
{
	int fd;
	void *kernel = NULL;

	if (argc != 2) {
		barebox_cmd_usage(cmdtp);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd > 0)
		kernel = (void *)memmap(fd, PROT_READ);

	if (!kernel)
		kernel = (void *)simple_strtoul(argv[1], NULL, 0);

	start_linux(kernel, 0, NULL);

	return 1;
}

static const __maybe_unused char cmd_bootu_help[] =
"Usage: bootu <address>\n";

BAREBOX_CMD_START(bootu)
	.cmd            = do_bootu,
	.usage          = "bootu - start a raw linux image",
	BAREBOX_CMD_HELP(cmd_bootu_help)
BAREBOX_CMD_END


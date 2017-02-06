#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <of.h>
#include <asm/armlinux.h>

static int do_bootu(int argc, char *argv[])
{
	int fd;
	void *kernel = NULL;
	void *oftree = NULL;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[1], O_RDONLY);
	if (fd > 0)
		kernel = (void *)memmap(fd, PROT_READ);

	if (!kernel)
		kernel = (void *)simple_strtoul(argv[1], NULL, 0);

#ifdef CONFIG_OFTREE
	oftree = of_get_fixed_tree(NULL);
#endif

	start_linux(kernel, 0, 0, 0, oftree, ARM_STATE_SECURE);

	return 1;
}

static const __maybe_unused char cmd_bootu_help[] =
"Usage: bootu <address>\n";

BAREBOX_CMD_START(bootu)
	.cmd            = do_bootu,
	BAREBOX_CMD_DESC("boot into already loaded Linux kernel")
	BAREBOX_CMD_OPTS("ADDRESS")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_bootu_help)
BAREBOX_CMD_END


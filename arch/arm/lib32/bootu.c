// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "bootu: " fmt

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <of.h>
#include <asm/armlinux.h>

static int do_bootu(int argc, char *argv[])
{
	int fd, ret;
	void *kernel = NULL;
	void *oftree = NULL;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[1], O_RDONLY);
	if (fd > 0)
		kernel = (void *)memmap(fd, PROT_READ);

	if (!kernel)
		kernel = (void *)simple_strtoul(argv[1], NULL, 0);

	oftree = of_get_fixed_tree_for_boot(NULL);
	if (!oftree) {
		pr_err("No devicetree given.\n");
		return -EINVAL;
	}

	ret = of_overlay_load_firmware();
	if (ret)
		return ret;

	start_linux(kernel, 0, 0, 0, oftree, ARM_STATE_SECURE, NULL);

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


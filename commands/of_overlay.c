// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2019 Michael Tretter <m.tretter@pengutronix.de>, Pengutronix

#include <command.h>
#include <common.h>
#include <environment.h>
#include <fdt.h>
#include <firmware.h>
#include <fs.h>
#include <getopt.h>
#include <libfile.h>
#include <of.h>
#include <linux/clk.h>

static int do_of_overlay(int argc, char *argv[])
{
	int ret;
	struct device_node *overlay;
	bool live_tree = false;
	int opt;

	while ((opt = getopt(argc, argv, "l")) > 0) {
		switch (opt) {
		case 'l':
			live_tree = true;
			break;
		}
	}

	if (argc != optind + 1)
		return COMMAND_ERROR_USAGE;

	if (live_tree && !IS_ENABLED(CONFIG_OF_OVERLAY_LIVE)) {
		printf("Support for live tree overlays is disabled. Enable CONFIG_OF_OVERLAY_LIVE\n");
		return 1;
	}

	overlay = of_read_file(argv[optind]);
	if (IS_ERR(overlay))
		return PTR_ERR(overlay);

	if (live_tree) {
		ret = of_overlay_apply_tree(of_get_root_node(), overlay);
		if (ret)
			goto err;

		of_clk_init();
		of_probe();
	} else {
		ret = of_register_overlay(overlay);
	}

	if (ret) {
		printf("cannot apply oftree overlay: %s\n", strerror(-ret));
		goto err;
	}

	return 0;

err:
	of_delete_node(overlay);
	return ret;
}

BAREBOX_CMD_HELP_START(of_overlay)
BAREBOX_CMD_HELP_TEXT("Register a device tree overlay file (dtbo) with barebox.")
BAREBOX_CMD_HELP_TEXT("By default the overlay is registered as a fixup and the")
BAREBOX_CMD_HELP_TEXT("overlay will then be applied to the Linux device tree.")
BAREBOX_CMD_HELP_TEXT("With -l given the overlay is applied to the barebox live")
BAREBOX_CMD_HELP_TEXT("tree instead. This involves probing new devices added in")
BAREBOX_CMD_HELP_TEXT("the overlay file.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-l", "apply to barebox live tree")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_overlay)
	.cmd = do_of_overlay,
	BAREBOX_CMD_DESC("register device tree overlay")
	BAREBOX_CMD_OPTS("[-l] FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_of_overlay_help)
BAREBOX_CMD_END

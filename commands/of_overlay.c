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

static int do_of_overlay(int argc, char *argv[])
{
	int ret;
	struct fdt_header *fdt;
	struct device_node *overlay;
	size_t size;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	fdt = read_file(argv[optind], &size);
	if (!fdt) {
		printf("cannot read %s\n", argv[optind]);
		return 1;
	}

	overlay = of_unflatten_dtb(fdt, size);
	free(fdt);
	if (IS_ERR(overlay))
		return PTR_ERR(overlay);

	ret = of_register_overlay(overlay);
	if (ret) {
		printf("cannot apply oftree overlay: %s\n", strerror(-ret));
		goto err;
	}

	return 0;

err:
	of_delete_node(overlay);
	return ret;
}

BAREBOX_CMD_START(of_overlay)
	.cmd = do_of_overlay,
	BAREBOX_CMD_DESC("register device tree overlay as fixup")
	BAREBOX_CMD_OPTS("FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

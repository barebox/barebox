// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <io.h>
#include <bbu.h>
#include <mach/socfpga/soc64-regs.h>

static int axe5_probe(struct device *dev)
{
	return 0;
}

static const struct of_device_id axe5_of_match[] = {
	{ .compatible = "arrow,axe5-eagle" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(axe5_of_match);

static struct driver axe5_board_driver = {
	.name = "board-arrow-axe5-eagle",
	.probe = axe5_probe,
	.of_compatible = axe5_of_match,
};
device_platform_driver(axe5_board_driver);

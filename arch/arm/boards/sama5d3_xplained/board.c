// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <envfs.h>

static int sama5d3_xplained_probe(struct device *dev)
{
	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_sama5d3_xplained);

	return 0;
}

static const struct of_device_id sama5d3_xplained_of_match[] = {
	{ .compatible = "atmel,sama5d3-xplained" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sama5d3_xplained_of_match);

static struct driver sama5d3_xplained_board_driver = {
	.name = "board-sama5d3_xplained",
	.probe = sama5d3_xplained_probe,
	.of_compatible = sama5d3_xplained_of_match,
};
coredevice_platform_driver(sama5d3_xplained_board_driver);

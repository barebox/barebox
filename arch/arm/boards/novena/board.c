// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 John Watts <contact@jookia.org>

#include <common.h>
#include <deep-probe.h>

static int novena_probe(struct device *dev)
{
	return 0;
}

static const struct of_device_id novena_of_match[] = {
	{ .compatible = "kosagi,imx6q-novena", },
	{ /* sentinel */ }
};

static struct driver novena_board_driver = {
	.name = "board-novena",
	.probe = novena_probe,
	.of_compatible = novena_of_match,
};
coredevice_platform_driver(novena_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(novena_of_match);

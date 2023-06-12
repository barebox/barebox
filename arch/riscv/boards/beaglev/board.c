// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <bbu.h>
#include <envfs.h>

static int beaglev_probe(struct device *dev)
{
	barebox_set_hostname("beaglev-starlight");

	defaultenv_append_directory(defaultenv_beaglev);

	return 0;
}

static const struct of_device_id beaglev_of_match[] = {
	{ .compatible = "beagle,beaglev-starlight-jh7100" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, beaglev_of_match);

static struct driver beaglev_board_driver = {
	.name = "board-beaglev",
	.probe = beaglev_probe,
	.of_compatible = beaglev_of_match,
};
device_platform_driver(beaglev_board_driver);

// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 Oleksij Rempel <o.rempel@pengutronix.de>
 *
 * Novarq Tactical 1000, a LAN9696 based 24x1G + 4x10G switch.
 *
 * This supports the pre-production revision, which is built as a close
 * copy of the Microchip EV23X71A reference design - hence the
 * novarq,tactical-1000-alpha compatible and a device tree that includes
 * the upstream EV23X71A one. The board is not manufactured by Microchip
 * and is not an EV23X71A, so it does not claim to be one, but it is
 * electrically that design.
 *
 * The shipping Tactical 1000 differs (fans, RTC, ...) and has a device
 * tree of its own. Supporting that board means adding a board entry
 * which includes it, not extending this one.
 */

#include <bbu.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>

static int tactical1000_probe(struct device *dev)
{
	bbu_register_std_file_update("nor", BBU_HANDLER_FLAG_DEFAULT,
				     "/dev/m25p0", filetype_fip);
	return 0;
}

static const struct of_device_id tactical1000_of_match[] = {
	{
		.compatible = "novarq,tactical-1000-alpha",
	},
	{ /* sentinel */ },
};

static struct driver tactical1000_board_driver = {
	.name = "board-novarq-tactical-1000",
	.probe = tactical1000_probe,
	.of_compatible = tactical1000_of_match,
};
coredevice_platform_driver(tactical1000_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(tactical1000_of_match);

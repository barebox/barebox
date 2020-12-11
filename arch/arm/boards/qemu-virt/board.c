// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Pengutronix e.K.
 *
 */
#include <common.h>
#include <init.h>
#include <asm/system_info.h>

static int virt_probe(struct device_d *dev)
{
	char *hostname = "virt";

	if (cpu_is_cortex_a7())
		hostname = "virt-a7";
	else if (cpu_is_cortex_a15())
		hostname = "virt-a15";

	barebox_set_model("ARM QEMU virt");
	barebox_set_hostname(hostname);

	return 0;
}

static const struct of_device_id virt_of_match[] = {
	{ .compatible = "linux,dummy-virt" },
	{ /* Sentinel */},
};

static struct driver_d virt_board_driver = {
	.name = "board-qemu-virt",
	.probe = virt_probe,
	.of_compatible = virt_of_match,
};

postcore_platform_driver(virt_board_driver);

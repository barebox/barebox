// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <driver.h>
#include <init.h>
#include <mach/zynqmp/zynqmp-bbu.h>
#include <deep-probe.h>

static int zcu102_probe(struct device *dev)
{
	return zynqmp_bbu_register_handler("SD", "/boot/BOOT.BIN",
					   BBU_HANDLER_FLAG_DEFAULT);
}

static const struct of_device_id zcu102_of_match[] = {
	{ .compatible = "xlnx,zynqmp-zcu102-revA" },
	{ .compatible = "xlnx,zynqmp-zcu102-revB" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(zcu102_of_match);

static struct driver zcu102_board_driver = {
	.name = "board-zynqmp-zcu102",
	.probe = zcu102_probe,
	.of_compatible = zcu102_of_match,
};
coredevice_platform_driver(zcu102_board_driver);

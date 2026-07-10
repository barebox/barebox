// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <deep-probe.h>
#include <mach/imx/bbu.h>

static int imx93_frdm_probe(struct device *dev)
{
	imx9_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", 0);

	return 0;
}

static const struct of_device_id frdm_imx93_of_match[] = {
	{
		.compatible = "fsl,imx93-11x11-frdm",
	},
        { /* sentinel */ },
};

BAREBOX_DEEP_PROBE_ENABLE(frdm_imx93_of_match);

static struct driver frdm_imx93_board_driver = {
	.name = "board-imx93-frdm",
	.probe = imx93_frdm_probe,
	.of_compatible = frdm_imx93_of_match,
};
coredevice_platform_driver(frdm_imx93_board_driver);

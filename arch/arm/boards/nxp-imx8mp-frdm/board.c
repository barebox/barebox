// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 Fabian Pfitzner, Pengutronix
 */

#include <asm/memory.h>
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/sizes.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx8mp.h>
#include <gpio.h>
#include <envfs.h>

static int nxp_imx8mp_frdm_probe(struct device *dev)
{
	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}

static const struct of_device_id nxp_imx8mp_frdm_of_match[] = {
	{ .compatible = "fsl,imx8mp-frdm" },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(nxp_imx8mp_frdm_of_match);

static struct driver nxp_imx8mp_frdm_board_driver = {
	.name = "board-nxp-imx8mp-frdm",
	.probe = nxp_imx8mp_frdm_probe,
	.of_compatible = nxp_imx8mp_frdm_of_match,
};
coredevice_platform_driver(nxp_imx8mp_frdm_board_driver);

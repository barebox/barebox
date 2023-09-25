// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/imx/bbu.h>

static int phyboard_polis_rdk_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;

	if (bootsource_get() == BOOTSOURCE_MMC && bootsource_get_instance() == 1) {
		of_device_enable_path("/chosen/environment-sd");
		sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", emmc_bbu_flag);
	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", sd_bbu_flag);

	return 0;
}

static const struct of_device_id phyboard_polis_rdk_of_match[] = {
	{ .compatible = "phytec,imx8mm-phyboard-polis-rdk" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(phyboard_polis_rdk_of_match);

static struct driver phyboard_polis_rdkboard_driver = {
	.name = "board-phyboard-polis-rdk",
	.probe = phyboard_polis_rdk_probe,
	.of_compatible = DRV_OF_COMPAT(phyboard_polis_rdk_of_match),
};
coredevice_platform_driver(phyboard_polis_rdkboard_driver);

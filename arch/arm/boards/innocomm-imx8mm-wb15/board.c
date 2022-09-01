// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/bbu.h>

static int innocomm_wb15_evk_probe(struct device_d *dev)
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

	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", emmc_bbu_flag);
	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", sd_bbu_flag);

	return 0;
}

static const struct of_device_id innocomm_wb15_evk_of_match[] = {
	{ .compatible = "innocomm,wb15-evk" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(innocomm_wb15_evk_of_match);

static struct driver_d innocomm_wb15_evkboard_driver = {
	.name = "board-innocomm-wb15-evk",
	.probe = innocomm_wb15_evk_probe,
	.of_compatible = DRV_OF_COMPAT(innocomm_wb15_evk_of_match),
};
coredevice_platform_driver(innocomm_wb15_evkboard_driver);

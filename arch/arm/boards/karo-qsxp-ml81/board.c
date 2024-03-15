// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/imx/bbu.h>

static int karo_qsxp_ml81_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;

	if (bootsource_get() == BOOTSOURCE_MMC && bootsource_get_instance() == 2) {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", emmc_bbu_flag);

	return 0;
}

static const struct of_device_id karo_qsxp_ml81_of_match[] = {
	{ .compatible = "karo,imx8mp-qsxp-ml81-qsbase4" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(karo_qsxp_ml81_of_match);

static struct driver karo_qsxp_ml81_board_driver = {
	.name = "board-karo-qsxp-ml81",
	.probe = karo_qsxp_ml81_probe,
	.of_compatible = DRV_OF_COMPAT(karo_qsxp_ml81_of_match),
};
coredevice_platform_driver(karo_qsxp_ml81_board_driver);

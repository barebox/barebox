// SPDX-License-Identifier: GPL-2.0

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx8mp.h>

static int skov_imx8mp_probe(struct device *dev)
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

	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", sd_bbu_flag);
	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", emmc_bbu_flag);

	return 0;
}

static const struct of_device_id skov_imx8mp_of_match[] = {
	/* generic, barebox specific compatible for all board variants */
	{ .compatible = "skov,imx8mp" },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(skov_imx8mp_of_match);

static struct driver skov_imx8mp_board_driver = {
	.name = "skov-imx8m",
	.probe = skov_imx8mp_probe,
	.of_compatible = skov_imx8mp_of_match,
};
coredevice_platform_driver(skov_imx8mp_board_driver);

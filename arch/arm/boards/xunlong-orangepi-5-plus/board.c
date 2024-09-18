// SPDX-License-Identifier: GPL-2.0-only
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/rockchip/bbu.h>

static int orangepi_5_plus_probe(struct device *dev)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT,
				  "/dev/mmc0");
	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc1");

	return 0;
}

static const struct of_device_id orangepi_5_plus_of_match[] = {
	{
		.compatible = "xunlong,orangepi-5-plus",
	},
	{ /* sentinel */ },
};

static struct driver orangepi_5_plus_board_driver = {
	.name = "board-orangepi-5-plus",
	.probe = orangepi_5_plus_probe,
	.of_compatible = orangepi_5_plus_of_match,
};
coredevice_platform_driver(orangepi_5_plus_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(orangepi_5_plus_of_match);

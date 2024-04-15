// SPDX-License-Identifier: GPL-2.0-only
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/rockchip/bbu.h>

struct rock5_model {
	const char *name;
	const char *shortname;
};

static int rock5_probe(struct device *dev)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();
	const struct rock5_model *model;

	model = device_get_match_data(dev);

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc0");
	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc1");

	return 0;
}

static const struct rock5_model rock5b = {
	.name = "Radxa ROCK5 Model B",
	.shortname = "rock5b",
};

static const struct of_device_id rock5_of_match[] = {
	{
		.compatible = "radxa,rock-5b",
		.data = &rock5b,
	},
	{ /* sentinel */ },
};

static struct driver rock5_board_driver = {
	.name = "board-rock5",
	.probe = rock5_probe,
	.of_compatible = rock5_of_match,
};
coredevice_platform_driver(rock5_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(rock5_of_match);

// SPDX-License-Identifier: GPL-2.0-only
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/rockchip/bbu.h>

struct rock3_model {
	const char *name;
	const char *shortname;
};

static int rock3_probe(struct device *dev)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();
	const struct rock3_model *model;

	model = device_get_match_data(dev);

	barebox_set_model(model->name);
	barebox_set_hostname(model->shortname);

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc0");
	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc1");

	return 0;
}

static const struct rock3_model rock3a = {
	.name = "Radxa ROCK3 Model A",
	.shortname = "rock3a",
};

static const struct of_device_id rock3_of_match[] = {
	{
		.compatible = "radxa,rock3a",
		.data = &rock3a,
	},
	{ /* sentinel */ },
};

static struct driver rock3_board_driver = {
	.name = "board-rock3",
	.probe = rock3_probe,
	.of_compatible = rock3_of_match,
};
coredevice_platform_driver(rock3_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(rock3_of_match);

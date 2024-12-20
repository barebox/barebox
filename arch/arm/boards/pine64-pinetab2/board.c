// SPDX-License-Identifier: GPL-2.0-only
#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <mach/rockchip/bbu.h>

struct pinetab2_model {
	const char *name;
	const char *shortname;
};

static int pinetab2_probe(struct device *dev)
{
	const struct pinetab2_model *model = device_get_match_data(dev);
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();

	barebox_set_model(model->name);
	barebox_set_hostname(model->shortname);

	if (bootsource == BOOTSOURCE_MMC && instance == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc0");
	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc1");

	return 0;
}

static const struct pinetab2_model pinetab2_v01 = {
	.name = "Pine64 PineTab 2 v0.1",
	.shortname = "pinetab2-v01",
};

static const struct pinetab2_model pinetab2_v20 = {
	.name = "Pine64 PineTab 2 v2.0",
	.shortname = "pinetab2-v20",
};

static const struct of_device_id pinetab2_of_match[] = {
	{ .compatible = "pine64,pinetab2-v0.1", .data = &pinetab2_v01, },
	{ .compatible = "pine64,pinetab2-v2.0", .data = &pinetab2_v20, },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(pinetab2_of_match);

static struct driver pinetab2_board_driver = {
	.name = "board-pinetab2",
	.probe = pinetab2_probe,
	.of_compatible = pinetab2_of_match,
};
coredevice_platform_driver(pinetab2_board_driver);

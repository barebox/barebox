// SPDX-License-Identifier: GPL-2.0-only
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/rockchip/bbu.h>

struct cm3_model {
	const char *name;
	const char *shortname;
};

static int cm3_probe(struct device *dev)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();
	const struct cm3_model *model;

	model = device_get_match_data(dev);

	barebox_set_model(model->name);
	barebox_set_hostname(model->shortname);

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT,
				  "/dev/mmc0");
	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc1");

	return 0;
}

static const struct cm3_model cm3_io = {
	.name = "Radxa CM3 on IO Board",
	.shortname = "cm3-io",
};

static const struct of_device_id cm3_of_match[] = {
	{
		.compatible = "radxa,cm3-io",
		.data = &cm3_io,
	},
	{ /* sentinel */ },
};

static struct driver cm3_io_board_driver = {
	.name = "board-cm3-io",
	.probe = cm3_probe,
	.of_compatible = cm3_of_match,
};
coredevice_platform_driver(cm3_io_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(cm3_of_match);

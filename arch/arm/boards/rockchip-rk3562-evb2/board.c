// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "rk3562-evb: " fmt

#include <common.h>
#include <init.h>
#include <mach/rockchip/bbu.h>
#include <aiodev.h>
#include <bootsource.h>
#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>
#include <deep-probe.h>

static int rk3562_evb2_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;

	if (bootsource_get() == BOOTSOURCE_MMC) {
		if (bootsource_get_instance() == 2)
			emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
		else
			sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	rockchip_bbu_mmc_register("sd", sd_bbu_flag, "/dev/mmc1");
	rockchip_bbu_mmc_register("emmc", emmc_bbu_flag, "/dev/mmc0");

	return 0;
}

static const struct of_device_id rk3562_evb2_of_match[] = {
	{ .compatible = "rockchip,rk3562-evb2-v10" },
	{ /* Sentinel */},
};

static struct driver rk3562_evb2_board_driver = {
	.name = "board-rk3562-evb",
	.probe = rk3562_evb2_probe,
	.of_compatible = rk3562_evb2_of_match,
};
coredevice_platform_driver(rk3562_evb2_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(rk3562_evb2_of_match);

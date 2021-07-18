// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <mach/bbu.h>
#include <bootsource.h>

static int rk3568_evb_probe(struct device_d *dev)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();

	barebox_set_model("Rockchip RK3568 EVB");
	barebox_set_hostname("rk3568-evb");

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rk3568_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/emmc.barebox");
	rk3568_bbu_mmc_register("sd", 0, "/dev/sd.barebox");

	return 0;
}

static const struct of_device_id rk3568_evb_of_match[] = {
	{ .compatible = "rockchip,rk3568-evb1-v10" },
	{ /* Sentinel */},
};

static struct driver_d rk3568_evb_board_driver = {
	.name = "board-rk3568-evb",
	.probe = rk3568_evb_probe,
	.of_compatible = rk3568_evb_of_match,
};
coredevice_platform_driver(rk3568_evb_board_driver);

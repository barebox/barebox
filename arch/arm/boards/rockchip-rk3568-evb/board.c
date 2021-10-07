// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "rk3568-evb: " fmt

#include <common.h>
#include <init.h>
#include <mach/bbu.h>
#include <aiodev.h>
#include <bootsource.h>
#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>

static bool machine_is_rk3568_evb = false;

static int rk3568_evb_probe(struct device_d *dev)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();

	barebox_set_model("Rockchip RK3568 EVB");
	barebox_set_hostname("rk3568-evb");
	machine_is_rk3568_evb = true;

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

static int rk3568_evb_detect_hwid(void)
{
	int ret;
	int evb_hwid_voltage;
	struct aiochannel *evb_hwid_chan;
	char *evb_hwid;

	if (!IS_ENABLED(CONFIG_AIODEV))
		return 0;

	if (!machine_is_rk3568_evb)
		return 0;

	evb_hwid_chan = aiochannel_by_name("aiodev0.in_value1_mV");
	if (IS_ERR(evb_hwid_chan)) {
		ret = PTR_ERR(evb_hwid_chan);
		goto err_hwid;
	}

	ret = aiochannel_get_value(evb_hwid_chan, &evb_hwid_voltage);
	if (ret)
		goto err_hwid;

	if (evb_hwid_voltage > 1650) {
		evb_hwid = "1";
	} else if (evb_hwid_voltage > 1350) {
		evb_hwid = "2";
	} else if (evb_hwid_voltage > 1050) {
		evb_hwid = "3";
	} else if (evb_hwid_voltage > 750) {
		evb_hwid = "4";
	} else if (evb_hwid_voltage > 450) {
		evb_hwid = "5";
	} else if (evb_hwid_voltage > 150) {
		evb_hwid = "6";
	} else {
		evb_hwid = "7";
	}
	pr_info("Detected RK3568 EVB%s\n", evb_hwid);

	globalvar_add_simple("board.hwid", evb_hwid);

	return 0;

err_hwid:
	pr_err("couldn't retrieve hardware ID");
	return ret;
}
late_initcall(rk3568_evb_detect_hwid);

BAREBOX_MAGICVAR(global.board.hwid, "The board hardware ID");

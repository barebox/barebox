// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "rk3568-r2pro: " fmt

#include <common.h>
#include <init.h>
#include <mach/rockchip/bbu.h>
#include <aiodev.h>
#include <bootsource.h>
#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>
#include <deep-probe.h>

static bool machine_is_bpi_r2pro = false;

static int rk3568_bpi_r2pro_probe(struct device *dev)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();

	barebox_set_model("BPI R2PRO");
	barebox_set_hostname("bpi-r2pro");
	machine_is_bpi_r2pro = true;

	if (bootsource == BOOTSOURCE_MMC && instance == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc0");
	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc1");

	return 0;
}

static const struct of_device_id rk3568_bpi_r2pro_of_match[] = {
	{ .compatible = "rockchip,rk3568-bpi-r2pro" },
	{ /* Sentinel */},
};

static struct driver rk3568_bpi_r2pro_board_driver = {
	.name = "board-rk3568-bpi-r2pro",
	.probe = rk3568_bpi_r2pro_probe,
	.of_compatible = rk3568_bpi_r2pro_of_match,
};
coredevice_platform_driver(rk3568_bpi_r2pro_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(rk3568_bpi_r2pro_of_match);

static int rk3568_bpi_r2pro_detect_hwid(void)
{
	int ret;
	int hwid_voltage;
	struct aiochannel *hwid_chan;
	char *hwid;

	if (!IS_ENABLED(CONFIG_AIODEV))
		return 0;

	if (!machine_is_bpi_r2pro)
		return 0;

	hwid_chan = aiochannel_by_name("aiodev0.in_value1_mV");
	if (IS_ERR(hwid_chan)) {
		ret = PTR_ERR(hwid_chan);
		goto err_hwid;
	}

	ret = aiochannel_get_value(hwid_chan, &hwid_voltage);
	if (ret)
		goto err_hwid;

	pr_info("hwid_voltage: %d\n", hwid_voltage);

	if (hwid_voltage == 1800)
		hwid = "V00";
	else
		hwid = "unknown";

	pr_info("Detected RK3568 BananaPi R2 Pro %s\n", hwid);

	globalvar_add_simple("board.hwid", hwid);

	return 0;

err_hwid:
	pr_err("couldn't retrieve hardware ID\n");
	return ret;
}
late_initcall(rk3568_bpi_r2pro_detect_hwid);

BAREBOX_MAGICVAR(global.board.hwid, "The board hardware ID");

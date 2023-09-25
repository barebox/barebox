// SPDX-License-Identifier: GPL-2.0-or-later
#include <common.h>
#include <driver.h>
#include <bootsource.h>
#include <mach/stm32mp/bbu.h>

static int phycore_stm32mp1_probe(struct device *dev)
{
	int sd_bbu_flags = 0, emmc_bbu_flags = 0;

	if (bootsource_get_instance() == 0) {
		of_device_enable_path("/chosen/environment-sd");
		sd_bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
	}

	stm32mp_bbu_mmc_register_handler("sd", "/dev/mmc0.ssbl", sd_bbu_flags);
	stm32mp_bbu_mmc_fip_register("emmc", "/dev/mmc1", emmc_bbu_flags);

	barebox_set_hostname("phyCORE-STM32MP1");

	return 0;
}

static const struct of_device_id phycore_stm32mp1_of_match[] = {
	{ .compatible = "phytec,phycore-stm32mp1-3" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, phycore_stm32mp1_of_match);

static struct driver phycore_stm32mp1_board_driver = {
	.name = "board-phycore-stm32mp1",
	.probe = phycore_stm32mp1_probe,
	.of_compatible = phycore_stm32mp1_of_match,
};
device_platform_driver(phycore_stm32mp1_board_driver);

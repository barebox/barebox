// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <init.h>
#include <mach/bbu.h>

static int dkx_probe(struct device_d *dev)
{
	stm32mp_bbu_mmc_register_handler("sd", "/dev/mmc0.ssbl",
					 BBU_HANDLER_FLAG_DEFAULT);

	barebox_set_model("STM32MP157C-DK2");

	return 0;
}

static const struct of_device_id dkx_of_match[] = {
	{ .compatible = "st,stm32mp157c-dk2" },
	{ /* sentinel */ },
};

static struct driver_d dkx_board_driver = {
	.name = "board-stm32mp15xx-dkx",
	.probe = dkx_probe,
	.of_compatible = dkx_of_match,
};
postcore_platform_driver(dkx_board_driver);

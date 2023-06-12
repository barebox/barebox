// SPDX-License-Identifier: GPL-2.0+

#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <mach/stm32mp/bbu.h>

static int ed1_probe(struct device *dev)
{
	int flags;

	flags = bootsource_get_instance() == 0 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("sd", "/dev/mmc0.ssbl", flags);

	flags = bootsource_get_instance() == 1 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("emmc", "/dev/mmc1.ssbl", flags);

	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	barebox_set_model("STM32MP157C-ED1");

	return 0;
}

/* ED1 is the SoM on top of the EV1 */
static const struct of_device_id ed1_of_match[] = {
	{ .compatible = "st,stm32mp157c-ed1" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ed1_of_match);

static struct driver ed1_board_driver = {
	.name = "board-stm32mp15x-ed1",
	.probe = ed1_probe,
	.of_compatible = ed1_of_match,
};
postcore_platform_driver(ed1_board_driver);

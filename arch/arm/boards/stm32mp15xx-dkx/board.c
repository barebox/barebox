// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <init.h>
#include <mach/stm32mp/bbu.h>
#include <deep-probe.h>

static int dkx_probe(struct device *dev)
{
	const void *model;

	stm32mp_bbu_mmc_register_handler("sd", "/dev/mmc0.ssbl",
					 BBU_HANDLER_FLAG_DEFAULT);

	if (dev_get_drvdata(dev, &model) == 0)
		barebox_set_model(model);

	barebox_set_hostname("stm32mp15xx-dkx");

	return 0;
}

static const struct of_device_id dkx_of_match[] = {
	{ .compatible = "st,stm32mp157a-dk1", .data = "STM32MP157A-DK1" },
	{ .compatible = "st,stm32mp157c-dk2", .data = "STM32MP157C-DK2" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(dkx_of_match);

static struct driver dkx_board_driver = {
	.name = "board-stm32mp15xx-dkx",
	.probe = dkx_probe,
	.of_compatible = dkx_of_match,
};
postcore_platform_driver(dkx_board_driver);

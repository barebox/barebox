// SPDX-License-Identifier: GPL-2.0+

#include <driver.h>
#include <init.h>
#include <mach/stm32mp/bbu.h>
#include <deep-probe.h>

static int stm32mp13xx_dk_probe(struct device *dev)
{
	stm32mp_bbu_mmc_fip_register("sd", "/dev/mmc0", BBU_HANDLER_FLAG_DEFAULT);
	return 0;
}

static const struct of_device_id stm32mp13xx_dk_of_match[] = {
	{ .compatible = "st,stm32mp135f-dk" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(stm32mp13xx_dk_of_match);

static struct driver stm32mp13xx_dk_board_driver = {
	.name = "board-stm32mp13xx_dk",
	.probe = stm32mp13xx_dk_probe,
	.of_compatible = stm32mp13xx_dk_of_match,
} ;
device_platform_driver(stm32mp13xx_dk_board_driver);

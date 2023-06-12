// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <linux/sizes.h>
#include <init.h>
#include <asm/memory.h>
#include <mach/stm32mp/bbu.h>
#include <bootsource.h>
#include <of.h>

static int odyssey_som_probe(struct device *dev)
{
	int flags;
	int instance = bootsource_get_instance();

	flags = instance == 0 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("sd", "/dev/mmc0.ssbl", flags);

	flags = instance == 1 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("emmc", "/dev/mmc1.ssbl", flags);


	if (instance == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	return 0;
}

static const struct of_device_id odyssey_som_of_match[] = {
	{ .compatible = "seeed,stm32mp157c-odyssey-som" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, odyssey_som_of_match);

static struct driver odyssey_som_driver = {
	.name = "odyssey-som",
	.probe = odyssey_som_probe,
	.of_compatible = odyssey_som_of_match,
};
device_platform_driver(odyssey_som_driver);

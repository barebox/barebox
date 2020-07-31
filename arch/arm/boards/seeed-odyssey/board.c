// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <linux/sizes.h>
#include <init.h>
#include <asm/memory.h>
#include <mach/bbu.h>
#include <bootsource.h>
#include <of.h>

static int odyssey_device_init(void)
{
	int flags;
	int instance = bootsource_get_instance();

	if (!of_machine_is_compatible("seeed,stm32mp157c-odyssey-som"))
		return 0;

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
device_initcall(odyssey_device_init);

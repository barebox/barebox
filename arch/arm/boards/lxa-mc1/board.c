// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <linux/sizes.h>
#include <init.h>
#include <asm/memory.h>
#include <mach/bbu.h>
#include <bootsource.h>

static int mc1_device_init(void)
{
	int flags;
	if (!of_machine_is_compatible("lxa,stm32mp157c-mc1"))
		return 0;

	flags = bootsource_get_instance() == 0 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("sd", "/dev/mmc0.ssbl", flags);

	flags = bootsource_get_instance() == 1 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("emmc", "/dev/mmc1.ssbl", flags);


	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	barebox_set_hostname("lxa-mc1");

	return 0;
}
device_initcall(mc1_device_init);

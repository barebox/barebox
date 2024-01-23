// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2011 Sascha Hauer, Pengutronix

#include <environment.h>
#include <bootsource.h>
#include <common.h>
#include <init.h>

#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <mach/imx/bbu.h>

static int tqma53_devices_init(void)
{
	char *of_env_path;
	unsigned bbu_flag_emmc = 0, bbu_flag_sd = 0;

	if (!of_machine_is_compatible("tq,tqma53"))
		return 0;

	barebox_set_hostname("tqma53");

	if (bootsource_get() == BOOTSOURCE_MMC &&
			bootsource_get_instance() == 1) {
		of_env_path = "/chosen/environment-sd";
		bbu_flag_sd = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		of_env_path = "/chosen/environment-emmc";
		bbu_flag_emmc = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx53_bbu_internal_mmc_register_handler("sd", "/dev/mmc1", bbu_flag_sd);
	imx53_bbu_internal_mmc_register_handler("emmc", "/dev/mmc2", bbu_flag_emmc);

	of_device_enable_path(of_env_path);

	armlinux_set_architecture(MACH_TYPE_TQMA53);

	return 0;
}
device_initcall(tqma53_devices_init);

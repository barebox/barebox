// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Christian Hemp
 */

#include <asm/memory.h>
#include <bootsource.h>
#include <environment.h>
#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <mach/bbu.h>

#include <envfs.h>

static int physom_imx8mq_devices_init(void)
{
	int flag_emmc = 0;
	int flag_sd = 0;

	if (!of_machine_is_compatible("phytec,imx8mq-pcl066"))
		return 0;

	barebox_set_hostname("phycore-imx8mq");

	switch (bootsource_get_instance()) {
	case 0:
		flag_emmc = BBU_HANDLER_FLAG_DEFAULT;
		of_device_enable_path("/chosen/environment-emmc");
		break;
	case 1:
	default:
		flag_sd = BBU_HANDLER_FLAG_DEFAULT;
		of_device_enable_path("/chosen/environment-sd");
		break;
	}

	imx8mq_bbu_internal_mmc_register_handler("eMMC",
						 "/dev/mmc0.barebox", flag_emmc);
	imx8mq_bbu_internal_mmc_register_handler("SD",
						 "/dev/mmc1.barebox", flag_sd);


	return 0;
}
device_initcall(physom_imx8mq_devices_init);

// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 */

#include <bootsource.h>
#include <common.h>
#include <envfs.h>
#include <init.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <mach/bbu.h>

static int zii_imx8mq_dev_init(void)
{
	if (!of_machine_is_compatible("zii,imx8mq-ultra"))
		return 0;

	barebox_set_hostname("imx8mq-zii-rdu3");

	imx8mq_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", 0);

	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-emmc");
	else
		of_device_enable_path("/chosen/environment-sd");

	defaultenv_append_directory(defaultenv_imx8mq_zii_dev);

	return 0;
}
device_initcall(zii_imx8mq_dev_init);

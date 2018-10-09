// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2017 Zodiac Inflight Innovation
 *
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * based on previous iterations of this code
 *
 *     Copyright (C) 2015 Nikita Yushchenko, CogentEmbedded, Inc
 *     Copyright (C) 2015 Andrey Gusakov, CogentEmbedded, Inc
 *
 * based on similar i.MX51 EVK (Babbage) board support code
 *
 *     Copyright (C) 2007 Sascha Hauer, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <mach/bbu.h>
#include <libfile.h>
#include <mach/imx5.h>

static int zii_rdu1_init(void)
{
	const char *hostname;

	if (!of_machine_is_compatible("zii,imx51-rdu1") &&
	    !of_machine_is_compatible("zii,imx51-scu2-mezz") &&
	    !of_machine_is_compatible("zii,imx51-scu3-esb"))
		return 0;

	hostname = of_get_machine_compatible() + strlen("imx51-");

	imx51_babbage_power_init();

	barebox_set_hostname(hostname);

	imx51_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", 0);
	imx51_bbu_internal_spi_i2c_register_handler("SPI",
		"/dev/dataflash0.barebox",
		BBU_HANDLER_FLAG_DEFAULT |
		IMX_BBU_FLAG_PARTITION_STARTS_AT_HEADER);

	return 0;
}
coredevice_initcall(zii_rdu1_init);

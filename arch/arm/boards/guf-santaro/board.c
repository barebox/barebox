/*
 * Copyright (C) 2014 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx6-regs.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/generic.h>
#include <sizes.h>
#include <bootsource.h>
#include <bbu.h>
#include <mach/bbu.h>
#include <mach/imx6.h>

static int santaro_postcore_init(void)
{
	if (!of_machine_is_compatible("guf,imx6q-santaro"))
		return 0;

	imx6_init_lowlevel();

	return 0;
}
postcore_initcall(santaro_postcore_init);

static int santaro_device_init(void)
{
	uint32_t flag_sd = 0, flag_emmc = 0;

	if (!of_machine_is_compatible("guf,imx6q-santaro"))
		return 0;

	barebox_set_hostname("santaro");

	writew(0x0, MX6_WDOG1_BASE_ADDR + 0x8);
	writew(0x0, MX6_WDOG2_BASE_ADDR + 0x8);

	if (bootsource_get() == BOOTSOURCE_MMC) {
		switch (bootsource_get_instance()) {
		case 1:
			flag_sd |= BBU_HANDLER_FLAG_DEFAULT;
			break;
		case 3:
			flag_emmc |= BBU_HANDLER_FLAG_DEFAULT;
			break;
		}
	}

	imx6_bbu_internal_mmc_register_handler("sd", "/dev/mmc1",
			flag_sd, NULL, 0, 0);
	imx6_bbu_internal_mmc_register_handler("emmc", "/dev/mmc3.boot0",
			flag_emmc, NULL, 0, 0);

	return 0;
}
device_initcall(santaro_device_init);

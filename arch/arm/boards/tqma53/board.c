/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <environment.h>
#include <bootsource.h>
#include <common.h>
#include <init.h>

#include <asm/armlinux.h>
#include <generated/mach-types.h>

static int tqma53_devices_init(void)
{
	char *of_env_path = "/chosen/environment-emmc";

	if (!of_machine_is_compatible("tq,tqma53"))
		return 0;

	barebox_set_model("TQ tqma53");
	barebox_set_hostname("tqma53");

	if (bootsource_get() == BOOTSOURCE_MMC &&
			bootsource_get_instance() == 1)
		of_env_path = "/chosen/environment-sd";

	of_device_enable_path(of_env_path);

	armlinux_set_architecture(MACH_TYPE_TQMA53);

	return 0;
}
device_initcall(tqma53_devices_init);

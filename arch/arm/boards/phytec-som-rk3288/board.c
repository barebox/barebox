/*
 * Copyright (C) 2016 PHYTEC Messtechnik GmbH,
 * Author: Wadim Egorov <w.egorov@phytec.de>
 *
 * Device initialization for the phyCORE-RK3288 SoM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <envfs.h>

static int physom_devices_init(void)
{
	if (!of_machine_is_compatible("phytec,rk3288-phycore-som"))
		return 0;

	barebox_set_hostname("pcm059");
	defaultenv_append_directory(defaultenv_physom_rk3288);

	return 0;
}
device_initcall(physom_devices_init);

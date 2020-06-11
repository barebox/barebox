// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2016 PHYTEC Messtechnik GmbH

/*
 * Author: Wadim Egorov <w.egorov@phytec.de>
 *
 * Device initialization for the phyCORE-RK3288 SoM
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

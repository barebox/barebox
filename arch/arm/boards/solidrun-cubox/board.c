// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>

#include <common.h>
#include <init.h>
#include <mach/bbu.h>

static int cubox_devices_init(void)
{
	if (!of_machine_is_compatible("solidrun,cubox"))
		return 0;

	mvebu_bbu_flash_register_handler("flash", "/dev/m25p0", 0, true);

	return 0;
}
device_initcall(cubox_devices_init);

/*
 * Copyright (C) 2013
 *  Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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
 *
 */

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

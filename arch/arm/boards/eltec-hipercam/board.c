/*
 * Copyright (C) 2015 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <bbu.h>
#include <mach/bbu.h>

static int hipercam_init(void)
{
	if (!of_machine_is_compatible("eltec,hipercam-rev01"))
		return 0;

	imx6_bbu_internal_spi_i2c_register_handler("nor", "/dev/m25p0.barebox",
			BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
device_initcall(hipercam_init);

/*
 * Copyright (C) 2019 Rouven Czerwinski, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */

#include <common.h>
#include <init.h>
#include <mach/generic.h>
#include <mach/bbu.h>

static int digi_ccimx6ulsbcpro_device_init(void)
{
	if (!of_machine_is_compatible("digi,ccimx6ulsbcpro"))
		return 0;

	imx6_bbu_nand_register_handler("nand", BBU_HANDLER_FLAG_DEFAULT);

	barebox_set_hostname("ccimx6ulsbcpro");

	return 0;
}
device_initcall(digi_ccimx6ulsbcpro_device_init);

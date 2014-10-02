/*
 * Copyright (C) 2014 Christian Hemp, Phytec Messtechnik GmbH
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

#include <envfs.h>
#include <environment.h>
#include <bootsource.h>
#include <common.h>
#include <gpio.h>
#include <init.h>
#include <of.h>

#include <mach/bbu.h>
#include <mach/imx6.h>

static int phytec_pcaaxl3_init(void)
{
	if (!of_machine_is_compatible("phytec,imx6q-pcaaxl3"))
		return 0;

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		of_device_enable_path("/chosen/environment-sd");
		break;
	default:
	case BOOTSOURCE_NAND:
		of_device_enable_path("/chosen/environment-nand");
		break;
	}

	imx6_bbu_nand_register_handler("nand", BBU_HANDLER_FLAG_DEFAULT);

	defaultenv_append_directory(defaultenv_phycard_imx6);

	return 0;
}
device_initcall(phytec_pcaaxl3_init);

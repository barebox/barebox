/**
 * @file
 * @brief GPMC specific NAND devices
 *
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
#include <console.h>
#include <init.h>
#include <driver.h>
#include <clock.h>
#include <io.h>

#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>

#define GPMC_CONF1_VALx8	0x00000800
#define GPMC_CONF1_VALx16	0x00001800

/**
 * @brief gpmc_generic_nand_devices_init - init generic nand device
 *
 * @return success/fail based on device function
 */
int omap_add_gpmc_nand_device(struct gpmc_nand_platform_data *pdata)
{
	if (pdata->device_width == 16)
		pdata->nand_cfg->cfg[0] = GPMC_CONF1_VALx16;
	else
		pdata->nand_cfg->cfg[0] = GPMC_CONF1_VALx8;

	/* Configure GPMC CS before register */
	gpmc_cs_config(pdata->cs, pdata->nand_cfg);

	add_generic_device("gpmc_nand", DEVICE_ID_DYNAMIC, NULL, (resource_size_t)omap_gpmc_base,
			1024 * 4, IORESOURCE_MEM, pdata);

	return 0;
}
